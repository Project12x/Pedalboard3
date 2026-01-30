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

4. **Create initial CHANGELOG.md** using Keep a Changelog format:
   ```markdown
   # Changelog
   
   All notable changes to this project will be documented in this file.
   
   The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
   
   ## [Unreleased]
   
   ### Added
   - Initial project structure
   ```

5. **Create README.md** with:
   - Project name and one-line description
   - Build instructions
   - License

6. **First commit BEFORE writing any code**
   ```bash
   git add .
   git commit -m "chore: Initial project structure"
   ```

7. **Create releases/ directory** (gitignored) for build archives

## Key Principle

> "Git first, code second. You can always add features, but you can't recover lost history."
