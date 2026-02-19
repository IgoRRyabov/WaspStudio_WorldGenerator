Add-Type -AssemblyName System.Drawing

# Annotation Drawer Script
# Put image and txt annotation file in "input" folder, run script, get result in "output" folder

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$inputDir = Join-Path $scriptDir "input"
$outputDir = Join-Path $scriptDir "output"

# Create folders if not exist
if (-not (Test-Path $inputDir)) { New-Item -ItemType Directory -Path $inputDir | Out-Null }
if (-not (Test-Path $outputDir)) { New-Item -ItemType Directory -Path $outputDir | Out-Null }

# Color map for classes
$colorMap = @{
    "M1A1" = "Red"; "M1A1_D" = "Red"
    "M1133" = "Orange"; "M1133_D" = "Orange"
    "T72B" = "Lime"; "T72B_D" = "Green"
    "M270" = "Blue"; "M270_D" = "DodgerBlue"
    "BUK_M1" = "Magenta"; "BUK_M1_D" = "Purple"
    "BMP3" = "Yellow"; "BMP3_D" = "Olive"
    "M109" = "Cyan"; "M109_D" = "Teal"
}

function Get-ColorForLabel($label) {
    foreach ($key in $colorMap.Keys) {
        if ($label -like "*$key*") {
            return $colorMap[$key]
        }
    }
    return "White"
}

# Find images
$imageFiles = Get-ChildItem -Path $inputDir -Include "*.jpg","*.jpeg","*.png","*.bmp" -File -Recurse
if ($imageFiles.Count -eq 0) {
    Write-Host "No images found in input folder!" -ForegroundColor Red
    Write-Host "Put image files (.jpg, .jpeg, .png, .bmp) in: $inputDir"
    exit 1
}

# Find annotation files
$txtFiles = Get-ChildItem -Path $inputDir -Include "*.txt" -File -Recurse
if ($txtFiles.Count -eq 0) {
    Write-Host "No annotation files (.txt) found in input folder!" -ForegroundColor Red
    Write-Host "Put annotation file in: $inputDir"
    exit 1
}

Write-Host "=== Annotation Drawer ===" -ForegroundColor Cyan
Write-Host "Found images: $($imageFiles.Count)"
Write-Host "Found annotations: $($txtFiles.Count)"
Write-Host ""

foreach ($imgFile in $imageFiles) {
    # Find matching txt file by name
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($imgFile.Name)
    $txtFile = $txtFiles | Where-Object { 
        [System.IO.Path]::GetFileNameWithoutExtension($_.Name) -eq $baseName 
    } | Select-Object -First 1
    
    if (-not $txtFile) {
        $txtFile = $txtFiles | Select-Object -First 1
        Write-Host "Using annotation for $($imgFile.Name): $($txtFile.Name)" -ForegroundColor Yellow
    }
    
    Write-Host "Processing: $($imgFile.Name)" -ForegroundColor Green
    
    # Load image
    $img = New-Object System.Drawing.Bitmap($imgFile.FullName)
    $graphics = [System.Drawing.Graphics]::FromImage($img)
    $font = New-Object System.Drawing.Font("Arial", 11, [System.Drawing.FontStyle]::Bold)
    $whiteBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
    
    # Read annotations
    $annotations = Get-Content $txtFile.FullName
    $boxCount = 0
    
    foreach ($line in $annotations) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        
        $parts = $line -split ","
        if ($parts.Count -lt 6) { continue }
        
        $label = $parts[0].Trim()
        $x1 = [int]$parts[2].Trim()
        $y1 = [int]$parts[3].Trim()
        $x2 = [int]$parts[4].Trim()
        $y2 = [int]$parts[5].Trim()
        
        $colorName = Get-ColorForLabel $label
        $color = [System.Drawing.Color]::FromName($colorName)
        $pen = New-Object System.Drawing.Pen($color, 2)
        $brush = New-Object System.Drawing.SolidBrush($color)
        
        $width = $x2 - $x1
        $height = $y2 - $y1
        
        # Draw rectangle
        $graphics.DrawRectangle($pen, $x1, $y1, $width, $height)
        
        # Draw label
        $textSize = $graphics.MeasureString($label, $font)
        $textY = $y1 - $textSize.Height - 2
        if ($textY -lt 0) { $textY = $y1 + 2 }
        
        $graphics.FillRectangle($brush, $x1, $textY, $textSize.Width, $textSize.Height)
        $graphics.DrawString($label, $font, $whiteBrush, $x1, $textY)
        
        $pen.Dispose()
        $brush.Dispose()
        $boxCount++
    }
    
    # Save result
    $outputPath = Join-Path $outputDir "$($baseName)_annotated.jpg"
    $img.Save($outputPath, [System.Drawing.Imaging.ImageFormat]::Jpeg)
    
    $graphics.Dispose()
    $img.Dispose()
    
    Write-Host "  -> Boxes drawn: $boxCount" -ForegroundColor Gray
    Write-Host "  -> Saved: $outputPath" -ForegroundColor Gray
}

$font.Dispose()
$whiteBrush.Dispose()

Write-Host ""
Write-Host "Done! Results in: $outputDir" -ForegroundColor Cyan
