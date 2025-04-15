#!/usr/bin/python3
import os
import sys
import time

# Generate dynamic HTML content
print("Content-Type: text/html\n")
print("<!DOCTYPE html>")
print("<html>")
print("<head><title>Dynamic Python CGI</title></head>")
print("<body>")
print("<h1>Python CGI Test</h1>")

# Show environment variables
print("<h2>Environment Info:</h2>")
print("<ul>")
print(f"<li>Server Name: {os.environ.get('SERVER_NAME', 'Unknown')}</li>")
print(f"<li>Server Port: {os.environ.get('SERVER_PORT', 'Unknown')}</li>")
print(f"<li>Request Method: {os.environ.get('REQUEST_METHOD', 'Unknown')}</li>")
print(f"<li>Current Time: {time.strftime('%Y-%m-%d %H:%M:%S')}</li>")
print("</ul>")

# Handle GET query parameters
if os.environ.get('REQUEST_METHOD') == 'GET':
    query = os.environ.get('QUERY_STRING', '')
    if query:
        print("<h2>Query Parameters:</h2>")
        print("<ul>")
        for param in query.split('&'):
            if '=' in param:
                key, value = param.split('=', 1)
                print(f"<li>{key}: {value}</li>")
        print("</ul>")

# Handle POST data
if os.environ.get('REQUEST_METHOD') == 'POST':
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length:
        post_data = sys.stdin.read(content_length)
        print("<h2>POST Data Received:</h2>")
        print(f"<pre>{post_data}</pre>")

print("</body>")
print("</html>")