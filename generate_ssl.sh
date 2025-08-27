#!/bin/bash
echo "Generating SSL certificate..."

pushd QLMSServer > /dev/null

# Generate private key
openssl genrsa -out server.key 2048

# Generate certificate signing request
openssl req -new -key server.key -out server.csr \
    -subj "/C=RO/ST=Bucharest/L=Sector 5/O=MTA/OU=FSISC/CN=localhost"

# Generate self-signed certificate
openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt

# Clean up CSR file
rm server.csr

popd > /dev/null

echo "SSL certificates generated successfully!"
echo "Files created in QLMSServer directory:"
echo "  - QLMSServer/server.key (private key)"
echo "  - QLMSServer/server.crt (certificate)"
