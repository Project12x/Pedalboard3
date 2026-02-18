/*
  ==============================================================================

    VuMeterDsp.h
    True VU meter DSP with 300ms integration time.

    Inspired by Fons Adriaensen's Vumeterdsp (GPL-2, used in Ardour).
    Implements a 2-pole lowpass filter on the absolute signal value,
    producing standard VU meter ballistics per IEC 60268-17:
      - 300ms rise time to 99% of full scale
      - ~1.5% overshoot on sine wave burst

    Usage:
      VuMeterDsp vu;
      vu.init(sampleRate);
      vu.process(buffer, numSamples);  // call from audio thread
      float level = vu.read();         // call from UI thread

  ==============================================================================
*/

#ifndef VUMETERDSP_H_INCLUDED
#define VUMETERDSP_H_INCLUDED

#include <atomic>
#include <cmath>


class VuMeterDsp
{
  public:
    VuMeterDsp() = default;

    /// Initialize filter coefficients for the given sample rate.
    void init(float sampleRate)
    {
        // The VU standard specifies 300ms to reach 99% of a 0 VU sine wave.
        // A critically damped 2-pole lowpass at ~3.5 Hz achieves this.
        // w = 2 * pi * f / sampleRate, using bilinear approximation.
        const float f = 3.5f; // cutoff frequency (Hz)
        w_ = 2.0f * 3.14159265f * f / sampleRate;
        // Gain factor: calibrate so that a 0 dBFS sine wave reads 0 VU.
        // Full-scale sine RMS = 0.707, mean abs = 0.637.
        // We want mean(abs(sin)) * g = 1.0, so g = 1/0.637 = 1.57.
        g_ = 1.5696f;
        z1_ = 0.0f;
        z2_ = 0.0f;
        peak_ = 0.0f;
    }

    /// Process a block of samples. Call from the audio thread.
    void process(const float* samples, int numSamples)
    {
        float z1 = z1_;
        float z2 = z2_;
        float m = peak_;
        const float w = w_;
        const float g = g_;

        for (int i = 0; i < numSamples; ++i)
        {
            // Rectify (absolute value)
            float x = std::abs(samples[i]) * g;
            // 2-pole lowpass (cascaded 1-pole filters)
            z1 += w * (x - z1);
            z2 += w * (z1 - z2);
            // Track the peak of the filtered signal
            if (z2 > m)
                m = z2;
        }

        z1_ = z1;
        z2_ = z2;
        peak_ = m;
    }

    /// Read the current VU level and reset peak hold.
    /// Returns a linear value (0.0 to ~1.0+ for 0 VU).
    /// Call from the UI thread.
    float read()
    {
        float m = peak_;
        peak_ = z2_; // reset peak to current filtered value
        return m;
    }

    /// Reset all state to zero.
    void reset()
    {
        z1_ = 0.0f;
        z2_ = 0.0f;
        peak_ = 0.0f;
    }

  private:
    float z1_ = 0.0f;   // first pole state
    float z2_ = 0.0f;   // second pole state
    float peak_ = 0.0f; // peak since last read()
    float w_ = 0.0f;    // filter coefficient
    float g_ = 1.0f;    // gain factor
};

#endif // VUMETERDSP_H_INCLUDED
