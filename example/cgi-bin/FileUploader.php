<?php
define("FILE_UPLOAD_LOCATION", "example/upload/"); // key, value
// $raw_data = $argv[1];

$raw_data = '';
while (($chunk = fread(STDIN, 8192)) != false && $chunk !== '')
{
    $raw_data .= $chunk;
}
if (empty($raw_data))
{
    fwrite(STDERR, "No data recieved.\n");
    exit(1);
}
$parsed_data = parseMultipart($raw_data);
if (!$parsed_data)
{
    fwrite(STDERR, "Invalid multipart data.\n");
    exit(1);
}
$file_name = $parsed_data['filename'];
$file_data = $parsed_data['file_data'];
$file_name = sanitizeFileName($file_name);
if (empty($file_name))
{
    fwrite(STDERR, "Invalid file name.\n");
    exit(1);
}
$file_name_size = strlen($file_name);
if ($file_name_size > 255)
{
    fwrite(STDERR, "File name too long.\n");
    exit(1);
}
if (!is_dir(FILE_UPLOAD_LOCATION))
{
    mkdir(FILE_UPLOAD_LOCATION, 0755, true);
}
$full_path = FILE_UPLOAD_LOCATION . $file_name;
$file_stream = fopen($full_path, "wb");
if (!$file_stream)
{
    fwrite(STDERR, "Failed to open file for writing.\n");
    exit(1);
}
if (fwrite($file_stream, $file_data) === false)
{
    fwrite(STDERR, "Failed to write file data.\n");
    fclose($file_stream);
    exit(1);
}
fclose($file_stream);
chmod($full_path, 0644);
echo "File uploaded successfull: $full_path\n";
exit(0);
function parseMultipart($raw_data)
{
    if (!preg_match('/^--(.+?)\r\n/', $raw_data, $matches))
    {
        fwrite(STDERR, "first match failed.\n");
        return false;
    }
    $boundry = $matches[1];
    $parts = explode("--$boundry", $raw_data);
    foreach ($parts as $part)
    {
        if (strpos($part, 'Content-Disposition: form-data; name="file"; filename="') !== false)
        {
            preg_match('/filename="(.+?)"/', $part, $name_matches);
            if (!isset($name_matches[1]))
            {
                fwrite(STDERR, "no /filename=\n");
                return false;
            }
            $filename = $name_matches[1];
            $file_data = substr($part, strpos($part, "\r\n\r\n") + 4);
            $file_data = rtrim($file_data, "\r\n\r\n");
            return ['filename' => $filename, 'file_data' => $file_data];
        }
    }
    fwrite(STDERR, 'no parts holding: Content-Disposition: form-data; name="file"; filename="\n');
    return false;
}
function sanitizeFileName($file_name)
{
    return preg_replace('/[^a-zA-Z0-9._-]/', '_', $file_name);
}