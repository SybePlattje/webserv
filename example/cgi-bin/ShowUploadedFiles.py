#!/usr/bin/python3

import os
from pathlib import Path
import sys
import html

def generateFormDirectory():
    directory = Path("./example/upload")
    html_content = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Files in {directory.name}</title>
    <style>
        body {{ font-family: Arial, sans-serif; padding 20px; }}
        h1 {{ color: #333; }}
        ul {{ line-height; 1.6; }}
        button {{ margin-left: 10px; }}
    </style>
</head>
<body>
    <h1>Files in {directory.resolve()}</h1>
    <ul>
"""
    if not directory.is_dir():
        html_content += "</ul></body></html>"
        return html_content
    
    files =[f.name for f in directory.iterdir() if f.is_file()]
    for file in files:
        safe_file = html.escape(file)
        html_content += f"<li>{safe_file} <button class='delete-btn' data-filename=\"{safe_file}\">Delete</button></li>\n"

    html_content += """
</ul>
<form action="/" method="get" id="back">
    <button type="submit">back to home</button>
</form>
<script>
    document.getElementById("back").addEventListener("submit", function(event) {
        event.preventDefault();
        window.location.href = this.action;
    });
    document.addEventListener("DOMContentLoaded", function() {
    const buttons = document.querySelectorAll('.delete-btn');
    buttons.forEach(button => {
        button.addEventListener("click", function() {
            const filename = button.getAttribute('data-filename');
            fetch('/upload/' + filename, {
                method: 'DELETE'
            })
            .then(response => {
                if (response.ok) {
                    alert("Deleted " + filename);
                    location.reload();
                } else {
                    alert("Failed to delete " + filename);
                }
            })
            .catch(err => {
                console.error("Request error:", err);
                alert("Error sending request.");
            });
        });
    });
});
</script>
</body>
<html>
"""
    return html_content

if __name__ == "__main__":
    try:
        html_string = generateFormDirectory()
        print(html_string)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)