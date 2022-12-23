#!/usr/bin/env python3
import os
import shutil
import sys

# Check if the correct number of arguments were provided
if len(sys.argv) != 2:
    print("Usage: python3 upload_file.py <URI> <file_data>")
    sys.exit(1)

# Get the URI and file data from the command-line arguments
uri = sys.argv[1]
file_data = sys.argv[2]

# Save the file to the specified URI
with open(uri, 'wb') as f:
    f.write(file_data.encode())

# Print a success message
print("File successfully uploaded to {}".format(uri))

#  use "./upload.py ./newfilecreated data" for test [WORKS]