#!/usr/bin/env python3
import sys
import uuid

# Check if the correct number of arguments were provided
if len(sys.argv) != 3:
    print("Usage: python3 upload.py <path> <file_data>")
    sys.exit(1)

# Get the URI and file data from the command-line arguments
uri = sys.argv[1]
filename = str(uuid.uuid4())
uri += filename
uri += ".txt"
file_data = sys.argv[2]

# Save the file to the specified URI
with open(uri, 'wb') as f:
    f.write(file_data.encode())

# Headers
output = "File successfully uploaded to {}".format(uri)
print("Content-Type: text/plain")
print("Content-Length:" + str(len(output)))

# Split body from headers
print("")

# Print body
print(output)
exit(0)