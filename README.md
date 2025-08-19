# Demo repository


## Getting Started

Before getting started, make sure you have a proper Zephyr development
environment. Follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

### Workspace setup

We need to create a Zephyr Workspace directory which will then contain both zephyr and our own code.

```shell
# Create workspace directory
mkdir <wherever-you-want-to-develop>
cd <same-directory-as-above>

# Check out this repository
git clone <URL> app

# Initialize zephyr workspace
west init --local app

# Fetch dependencies
west update

# Install python dependencies
# Optional, insert here: enter some virtual environment
pip install -r zephyr/scripts/requirements.txt

# Fetch required SDK
(cd zephyr; west sdk install -t arm-zephyr-eabi x86_64-zephyr-elf aarch64-zephyr-elf)

# Open the app workspace in VSCode
code ./app/app.code-workspace
```
