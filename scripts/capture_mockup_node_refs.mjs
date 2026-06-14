import { chromium } from "playwright";
import { createReadStream } from "node:fs";
import http from "node:http";
import { mkdir } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(__dirname, "..");

const args = new Map();
for (let i = 2; i < process.argv.length; i += 1) {
  const arg = process.argv[i];
  if (!arg.startsWith("--")) continue;
  const eq = arg.indexOf("=");
  if (eq >= 0) args.set(arg.slice(2, eq), arg.slice(eq + 1));
  else args.set(arg.slice(2), process.argv[i + 1]?.startsWith("--") ? "true" : process.argv[++i] ?? "true");
}

const outDir = path.resolve(repoRoot, args.get("out") || "documentation/qa/node-reference");
const theme = args.get("theme") || "midnight";
const browserChannel = args.get("browser") || args.get("channel") || "msedge";
const requestedNodes = (args.get("nodes") || "nam-loader,ir-loader,effect-rack,tuner,mixer,splitter")
  .split(",")
  .map((name) => name.trim())
  .filter(Boolean);

const referencePage = path.join(
  repoRoot,
  "documentation",
  "mockup-main-window-v2-lXKi",
  "pedalboard-remix",
  "project",
  "Node Reference.html",
);

function getContentType(filePath) {
  const ext = path.extname(filePath).toLowerCase();
  if (ext === ".html") return "text/html; charset=utf-8";
  if (ext === ".css") return "text/css; charset=utf-8";
  if (ext === ".js" || ext === ".jsx" || ext === ".mjs") return "text/javascript; charset=utf-8";
  if (ext === ".png") return "image/png";
  if (ext === ".svg") return "image/svg+xml";
  if (ext === ".woff2") return "font/woff2";
  return "application/octet-stream";
}

async function startStaticServer(rootDir) {
  const root = path.resolve(rootDir);
  const server = http.createServer((request, response) => {
    const requestUrl = new URL(request.url || "/", "http://127.0.0.1");
    const relativePath = decodeURIComponent(requestUrl.pathname).replace(/^[/\\]+/, "");
    const filePath = path.resolve(root, relativePath);

    if (filePath !== root && !filePath.startsWith(root + path.sep)) {
      response.writeHead(403);
      response.end("Forbidden");
      return;
    }

    response.setHeader("Content-Type", getContentType(filePath));
    const stream = createReadStream(filePath);
    stream.on("error", () => {
      if (!response.headersSent) response.writeHead(404);
      response.end("Not found");
    });
    stream.pipe(response);
  });

  await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
  const address = server.address();
  return {
    server,
    urlFor(filePath) {
      const rel = path.relative(root, filePath).split(path.sep).map(encodeURIComponent).join("/");
      return `http://127.0.0.1:${address.port}/${rel}`;
    },
  };
}

await mkdir(outDir, { recursive: true });

const staticServer = await startStaticServer(repoRoot);
let browser;
try {
  browser = await chromium.launch({ headless: true, channel: browserChannel });
} catch (error) {
  console.warn(`[mockup-node-ref] Could not launch ${browserChannel}; falling back to Playwright Chromium.`);
  console.warn(`[mockup-node-ref] ${error.message}`);
  browser = await chromium.launch({ headless: true });
}
try {
  const context = await browser.newContext({
    viewport: { width: 1440, height: 1400 },
    deviceScaleFactor: 1,
  });
  const page = await context.newPage();
  const url = `${staticServer.urlFor(referencePage)}?theme=${encodeURIComponent(theme)}`;
  await page.goto(url);
  await page.waitForSelector("[data-node-ref] .m2-node", { state: "visible" });

  for (const nodeName of requestedNodes) {
    const tile = page.locator(`[data-node-ref="${nodeName}"]`);
    await tile.waitFor({ state: "visible" });
    const outputPath = path.join(outDir, `mockup-node-${nodeName}.png`);
    await tile.screenshot({ path: outputPath });
    console.log(`[mockup-node-ref] ${nodeName} -> ${path.relative(repoRoot, outputPath)}`);
  }
} finally {
  await browser.close();
  staticServer.server.close();
}
