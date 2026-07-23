# Pre-stage arduino-cli dependencies into the download cache using curl --resolve,
# bypassing corporate DNS filtering of arduino.cc / GitHub asset CDN hosts.
# arduino-cli then installs from cache without needing to resolve those hosts.
$ErrorActionPreference = 'Stop'

$data  = "$env:LOCALAPPDATA\Arduino15"
$stage = Join-Path $data 'staging\packages'
New-Item -ItemType Directory -Force -Path $stage | Out-Null

# Resolve hosts that corporate DNS blocks (and CDN hosts) via public DNS 8.8.8.8.
$hostsToResolve = @(
  'downloads.arduino.cc',
  'release-assets.githubusercontent.com',
  'objects.githubusercontent.com',
  'github.com',
  'codeload.github.com'
)
$resolveArgs = @()
foreach ($h in $hostsToResolve) {
  $ip = $null
  try { $ip = (Resolve-DnsName $h -Type A -Server 8.8.8.8 -ErrorAction Stop | Where-Object IPAddress | Select-Object -First 1).IPAddress } catch {}
  if (-not $ip) { try { $ip = (Resolve-DnsName $h -Type A -ErrorAction Stop | Where-Object IPAddress | Select-Object -First 1).IPAddress } catch {} }
  if ($ip) {
    $resolveArgs += @('--resolve', "${h}:443:$ip", '--resolve', "${h}:80:$ip")
    Write-Host ("resolve {0,-40} -> {1}" -f $h, $ip)
  } else {
    Write-Host ("resolve {0,-40} -> (using system DNS)" -f $h)
  }
}

function Get-ExpectedSha([string]$checksum) {
  if ($checksum -match '^SHA-256:(.+)$') { return $Matches[1].ToLower() }
  return $null
}

function Fetch-Archive($url, $fileName, $checksum, $size) {
  $dest = Join-Path $stage $fileName
  $expected = Get-ExpectedSha $checksum
  if (Test-Path $dest) {
    $have = (Get-FileHash $dest -Algorithm SHA256).Hash.ToLower()
    if ($expected -and $have -eq $expected) {
      Write-Host ("  cached OK  {0}" -f $fileName); return $true
    }
    Remove-Item $dest -Force
  }
  Write-Host ("  download   {0}  ({1:N1} MB)" -f $fileName, ($size/1MB))
  $args = @('-sSL','--ssl-no-revoke') + $resolveArgs + @('-o', $dest, '-w', 'http=%{http_code}\n', $url, '--max-time', '1800')
  $out = & curl.exe @args
  if (Test-Path $dest) {
    if ($expected) {
      $have = (Get-FileHash $dest -Algorithm SHA256).Hash.ToLower()
      if ($have -ne $expected) { Write-Host ("  CHECKSUM MISMATCH {0} ({1})" -f $fileName, $out) -ForegroundColor Red; return $false }
    }
    Write-Host ("  ok         {0} {1}" -f $fileName, $out.Trim())
    return $true
  }
  Write-Host ("  FAILED     {0} {1}" -f $fileName, $out) -ForegroundColor Red
  return $false
}

$fail = 0

# --- ESP32 platform + Windows tools -----------------------------------------
$ej = Get-Content "$data\package_esp32_index.json" -Raw | ConvertFrom-Json
$epkg = $ej.packages | Where-Object { $_.name -eq 'esp32' }
$plat = $epkg.platforms | Where-Object { $_.version -eq '3.3.11' } | Select-Object -First 1
Write-Host "== ESP32 platform =="
if (-not (Fetch-Archive $plat.url $plat.archiveFileName $plat.checksum $plat.size)) { $fail++ }

Write-Host "== ESP32 tools (Windows) =="
foreach ($td in $plat.toolsDependencies) {
  $tool = $epkg.tools | Where-Object { $_.name -eq $td.name -and $_.version -eq $td.version } | Select-Object -First 1
  if (-not $tool) { continue }
  $sys = $tool.systems | Where-Object { $_.host -eq 'x86_64-mingw32' } | Select-Object -First 1
  if (-not $sys) { $sys = $tool.systems | Where-Object { $_.host -match 'mingw32|w64-mingw|windows' } | Select-Object -First 1 }
  if (-not $sys) { Write-Host ("  skip (no win) {0}" -f $td.name); continue }
  if (-not (Fetch-Archive $sys.url $sys.archiveFileName $sys.checksum $sys.size)) { $fail++ }
}

# --- Builtin tools (ctags etc.) from base index -----------------------------
$bj = Get-Content "$data\package_index.json" -Raw | ConvertFrom-Json
$bpkg = $bj.packages | Where-Object { $_.name -eq 'builtin' }
$need = @{
  'serial-discovery'='1.5.2'; 'mdns-discovery'='1.1.0'; 'serial-monitor'='0.15.0';
  'ctags'='5.8-arduino11'; 'dfu-discovery'='0.1.2'
}
Write-Host "== Builtin tools (Windows) =="
foreach ($tool in $bpkg.tools) {
  if (-not $need.ContainsKey($tool.name)) { continue }
  if ($tool.version -ne $need[$tool.name]) { continue }
  $sys = $tool.systems | Where-Object { $_.host -eq 'x86_64-mingw32' -or $_.archiveFileName -match 'Windows_64bit' } | Select-Object -First 1
  if (-not $sys) { $sys = $tool.systems | Where-Object { $_.host -match 'mingw32|windows' } | Select-Object -First 1 }
  if (-not $sys) { Write-Host ("  skip (no win) {0}" -f $tool.name); continue }
  if (-not (Fetch-Archive $sys.url $sys.archiveFileName $sys.checksum $sys.size)) { $fail++ }
}

Write-Host "============================================"
if ($fail -gt 0) { Write-Host "$fail archive(s) FAILED" -ForegroundColor Red; exit 1 }
Write-Host "All archives staged OK." -ForegroundColor Green
