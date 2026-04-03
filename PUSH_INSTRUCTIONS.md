# Push Instructions for GitHub Fork

## 1. Create Fork on GitHub

1. Go to: https://github.com/HASwitchPlate/openHASP
2. Click the **"Fork"** button (top right)
3. Select your account: **nopenix**
4. Wait for the fork to be created

## 2. Add Your Fork as Remote

```bash
cd /config/workspace/git/openHASP

# Add your fork as remote (if not already done)
git remote add fork https://github.com/nopenix/openHASP.git

# Verify remotes
git remote -v
```

## 3. Push to Your Fork

```bash
# Push master branch to your fork
git push fork master

# Or force push if needed (be careful!)
# git push fork master --force
```

## 4. Set Fork as Default (Optional)

```bash
# Set your fork as the default push remote
git branch --set-upstream-to=fork/master master
```

## 5. Verify on GitHub

1. Visit: https://github.com/nopenix/openHASP
2. Check that your changes are visible:
   - README.md should show the fork-specific header
   - CHANGES.md should be present
   - Dockerfile.platformio should be present
   - build-docker.sh should be present

## 6. Create a Release (Optional but Recommended)

1. Go to: https://github.com/nopenix/openHASP/releases
2. Click **"Create a new release"**
3. Tag version: `v0.7.0.2-NopeNix`
4. Release title: `Sunton 8048S043R Support`
5. Description: Copy from CHANGES.md
6. Attach firmware binaries from `build_output/firmware/`

## Quick Reference

```bash
# Full workflow
git remote add fork https://github.com/nopenix/openHASP.git
git push fork master

# Future updates
git add -A
git commit -m "Your commit message"
git push fork master
```

## Troubleshooting

### Authentication Issues
If you get 403/401 errors, you need to authenticate:
- Use HTTPS with personal access token
- Or set up SSH keys: https://docs.github.com/en/authentication/connecting-to-github-with-ssh

### Submodule Issues
```bash
# If submodules are not pushed
git submodule update --init --recursive
git push fork --recurse-submodules=on-demand
```

### Large File Issues
```bash
# If push fails due to file size
git lfs track "*.bin"
git add .gitattributes
git commit -m "Track binary files with LFS"
```
