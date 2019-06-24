###############################################################################
# General Setup
###############################################################################
# Install python3 and python dev files
sudo apt-get install python3 python3-dev

# Install pip3 and python3-virtualenv
sudo apt-get install python3-pip python3-virtualenv

# Create Virtual Environment
virtualenv -p /usr/bin/python3 venv

# Activate Virtual Environment
source ./venv/bin/activate

# Install package requirements
pip install -r requirements.txt --user

# Download, Checkout, and Install Pymodbus
git clone https://github.com/bashwork/pymodbus
git checkout --track origin/python3
python setup.py install

# Run Program
python modbusgen.py

# Exit Virtual Environment
deactivate
