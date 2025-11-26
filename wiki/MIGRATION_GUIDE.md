# Migrating Documentation to GitHub Wiki

This document contains instructions for transferring documentation to GitHub Wiki.

## Files for Wiki

The `wiki/` directory contains the following files:

| File | Description |
|------|-------------|
| `Home.md` | Wiki landing page |
| `_Sidebar.md` | Navigation sidebar |
| `API.md` | Complete API reference |
| `BUILD.md` | Build instructions |
| `TUTORIAL.md` | Step-by-step guide |
| `CONTRIBUTING.md` | Contribution guidelines |

## Migration Steps

### Method 1: Via GitHub Web Interface

1. Navigate to the repository on GitHub
2. Click on the **Wiki** tab
3. Click **Create the first page** (if Wiki is empty) or **New Page**
4. For each file in `wiki/`:
   - Create a page with the corresponding name (without .md extension)
   - Copy the file contents
   - Click **Save Page**

### Method 2: Via git (Recommended)

GitHub Wiki is a separate git repository. You can clone it and push changes:

```bash
# Clone the Wiki repository
git clone https://github.com/fica99/HotPlugPP.wiki.git

# Copy files
cd HotPlugPP.wiki
cp ../HotPlugPP/wiki/* .

# Add and commit
git add .
git commit -m "Migrate documentation to Wiki"

# Push to Wiki
git push origin master
```

> **Note:** The Wiki repository becomes available after creating the first page via the web interface.

## Page Creation Order

Recommended order:

1. `Home` — Landing page (must be first)
2. `_Sidebar` — Navigation sidebar
3. `BUILD` — Build instructions
4. `TUTORIAL` — Tutorial
5. `API` — API Reference
6. `CONTRIBUTING` — Contributor guidelines

## Updating README.md

After migration, consider updating links in the repository's README.md to point to Wiki:

```markdown
## Documentation

- 📖 **[Wiki](../../wiki)** — Full documentation
- 📦 **[Build](../../wiki/BUILD)** — Build instructions
- 📝 **[Tutorial](../../wiki/TUTORIAL)** — Creating your first plugin
- 📚 **[API Reference](../../wiki/API)** — API documentation
- 🤝 **[Contributing](../../wiki/CONTRIBUTING)** — Contributor guidelines
```

## Post-Migration Checklist

After migration, verify:

1. ✅ All pages are created
2. ✅ Sidebar is displayed
3. ✅ Links between pages work
4. ✅ Code formatting displays correctly
5. ✅ Tables render properly

## Documentation Improvements Made

During Wiki preparation, the following improvements were made:

1. **Fixed header file names in examples**:
   - `IPlugin.hpp` → `i_plugin.hpp`
   - `PluginLoader.hpp` → `plugin_loader.hpp`

2. **Updated wiki links** to use GitHub Wiki syntax

3. **Streamlined content** to focus on stable API elements
