#!/usr/bin/php
<?php
header('Content-Type: text/html');

echo "<!DOCTYPE html>
<html>
<head>
    <title>File Upload Handler</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .upload-form { margin: 20px 0; padding: 20px; border: 1px solid #ccc; }
        .response { margin: 20px 0; padding: 10px; background: #f0f0f0; }
    </style>
</head>
<body>
    <h1>File Upload Test</h1>";

// Display upload form
echo "<div class='upload-form'>
    <h2>Upload a File</h2>
    <form action='/cgi-bin/FileUpload.php' method='post' enctype='multipart/form-data'>
        <input type='file' name='uploadfile' required>
        <input type='submit' value='Upload'>
    </form>
</div>";

// Handle file upload
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    echo "<div class='response'>";
    if (isset($_FILES['uploadfile'])) {
        $upload_dir = '../uploads/';
        if (!is_dir($upload_dir)) {
            mkdir($upload_dir, 0777, true);
        }

        $file = $_FILES['uploadfile'];
        $target_path = $upload_dir . basename($file['name']);

        if (move_uploaded_file($file['tmp_name'], $target_path)) {
            echo "<h3>File Upload Success</h3>";
            echo "<p>File: " . htmlspecialchars($file['name']) . "</p>";
            echo "<p>Size: " . number_format($file['size']) . " bytes</p>";
            echo "<p>Type: " . htmlspecialchars($file['type']) . "</p>";
        } else {
            echo "<h3>Upload Failed</h3>";
            echo "<p>Error: Unable to save file</p>";
        }
    } else {
        echo "<h3>No File Uploaded</h3>";
        echo "<p>Please select a file to upload.</p>";
    }
    echo "</div>";
}

// Show existing uploads
$upload_dir = '../uploads/';
if (is_dir($upload_dir)) {
    echo "<div class='response'>
        <h2>Existing Uploads</h2>
        <ul>";
    
    $files = array_diff(scandir($upload_dir), array('.', '..'));
    foreach ($files as $file) {
        echo "<li>" . htmlspecialchars($file) . "</li>";
    }
    
    echo "</ul></div>";
}

echo "</body></html>";
?>