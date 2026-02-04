---
description: Initialize a new project with proper structure from day one
---

# New Project Workflow

Start projects correctly to avoid losing history and build artifacts.

## Steps

1. **Create project directory and initialize git FIRST**
   ```bash
   mkdir ProjectName
   cd ProjectName
   git init
   ```

2. **Create minimal structure**
   ```
   ProjectName/
   ├── .gitignore
   ├── README.md
   ├── CHANGELOG.md
   └── src/
   ```

3. **Create .gitignore** with common exclusions:
   ```
   # Build artifacts
   build/
   out/
   *.exe
   *.dll
   *.obj
   
   # IDE
   .vs/
   .vscode/
   *.user
   
   # Archives (local only)
   releases/
   ```

4. **Language Server Sanity** - Prevent 10-20GB memory bloat:

   **For C++ projects**, create `.clangd`:
   ```yaml
   If:
     PathExclude:
       - .*/build/.*
       - .*/out/.*
       - .*/_deps/.*
       - .*/third_party/.*
       - .*/external/.*
       - .*/vendor/.*
       # JUCE projects: also add these
       # - .*/JUCE/extras/.*
       # - .*/JUCE/modules/.*/native/.*
       # - .*/JuceLibraryCode/.*

   Index:
     Background: Build

   Diagnostics:
     UnusedIncludes: None
   ```

   **For Node.js/React projects**, create `jsconfig.json` or ensure `tsconfig.json` has:
   ```json
   {
     "compilerOptions": {
       "checkJs": false
     },
     "exclude": [
       "node_modules",
       "dist",
       "build",
       ".next",
       "coverage"
     ]
   }
   ```

   **For Python projects**, create `pyrightconfig.json`:
   ```json
   {
     "exclude": [
       "venv",
       ".venv",
       "__pycache__",
       "build",
       "dist",
       ".eggs"
     ]
   }
   ```

   **For Rust projects**, no config needed—rust-analyzer is well-behaved.

   **For Go projects**, no config needed—gopls is well-behaved.

   > **Key Principle:** Always exclude `build/`, `dist/`, `node_modules/`, `venv/`, and vendored dependencies from language server indexing.

5. **Create initial CHANGELOG.md** using Keep a Changelog format:
   ```markdown
   # Changelog
   
   All notable changes to this project will be documented in this file.
   
   The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
   
   ## [Unreleased]
   
   ### Added
   - Initial project structure
   ```

6. **Create README.md** with:
   - Project name and one-line description
   - Build instructions
   - License

7. **First commit BEFORE writing any code**
   ```bash
   git add .
   git commit -m "chore: Initial project structure"
   ```

8. **Create releases/ directory** (gitignored) for build archives

## Key Principle

> "Git first, code second. You can always add features, but you can't recover lost history."
