#!/bin/bash
# Setup script for Parquet Explorer

set -e

echo "=================================="
echo "Parquet Explorer Setup"
echo "=================================="

# Check if we're in the right directory
if [ ! -f "requirements.txt" ]; then
    echo "Error: requirements.txt not found. Please run this script from the parquet_explorer directory."
    exit 1
fi

# Create virtual environment
echo ""
echo "Creating virtual environment..."
python3 -m venv .venv

# Activate virtual environment
echo ""
echo "Activating virtual environment..."
source .venv/bin/activate

# Upgrade pip
echo ""
echo "Upgrading pip..."
pip install --upgrade pip

# Install dependencies
echo ""
echo "Installing dependencies..."
pip install -r requirements.txt

# Make the script executable
chmod +x parquet_explorer.py

echo ""
echo "=================================="
echo "Setup Complete!"
echo "=================================="
echo ""
echo "To use the Parquet Explorer:"
echo ""
echo "1. Activate the virtual environment:"
echo "   source .venv/bin/activate"
echo ""
echo "2. Run the explorer:"
echo "   python parquet_explorer.py /path/to/your/parquet/files"
echo ""
echo "Example:"
echo "   python parquet_explorer.py /tmp/explorer_test/daily/"
echo ""
