# Renders CJK glyphs with SimHei via GDI+ and prints packed bitmap rows
# matching cjk_font.h format: height rows x 2 bytes/row, MSB first, adv=12 (12px) / 16 (16px)
Add-Type -AssemblyName System.Drawing

# codepoints: 0x6E7F, 0x5361, 0x7247, 0x6280, 0x79D1
$cps = @(0x6E7F, 0x5361, 0x7247, 0x6280, 0x79D1)

foreach ($size in @(12, 16)) {
    Write-Output "=== SIZE $size ==="
    foreach ($cp in $cps) {
        $ch = [char]$cp
        $bmp = New-Object System.Drawing.Bitmap($size, $size)
        $gfx = [System.Drawing.Graphics]::FromImage($bmp)
        $gfx.Clear([System.Drawing.Color]::White)
        $font = New-Object System.Drawing.Font('SimHei', $size, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
        $brush = [System.Drawing.Brushes]::Black
        $fmt = New-Object System.Drawing.StringFormat
        $fmt.Alignment = 'Center'
        $fmt.LineAlignment = 'Center'
        $rect = New-Object System.Drawing.RectangleF(0, 0, $size, $size)
        $gfx.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::SingleBitPerPixelGridFit
        $gfx.DrawString($ch.ToString(), $font, $brush, $rect, $fmt)
        $gfx.Dispose()

        $bytes = @()
        for ($row = 0; $row -lt $size; $row++) {
            $b0 = 0; $b1 = 0
            for ($col = 0; $col -lt $size; $col++) {
                $p = $bmp.GetPixel($col, $row)
                if ($p.GetBrightness() -lt 0.5) {
                    if ($col -lt 8) { $b0 = $b0 -bor (0x80 -shr $col) }
                    else { $b1 = $b1 -bor (0x80 -shr ($col - 8)) }
                }
            }
            $bytes += $b0; $bytes += $b1
        }
        $hex = ($bytes | ForEach-Object { '0x{0:X2}' -f $_ }) -join ','
        Write-Output ("{0}|{1}|{2}" -f $cp, $size, $hex)
        $bmp.Dispose(); $font.Dispose()
    }
}
