param(
	[string]$Goal = $(if ($env:OFXGGML_AGENTS_GOAL) { $env:OFXGGML_AGENTS_GOAL } else { "" }),
	[string]$Prompt = $(if ($env:OFXGGML_AGENTS_PROMPT) { $env:OFXGGML_AGENTS_PROMPT } else { "" }),
	[string]$Model = $(if ($env:OFXGGML_AGENTS_MODEL) { $env:OFXGGML_AGENTS_MODEL } else { "" }),
	[string]$Tools = $(if ($env:OFXGGML_AGENTS_TOOLS) { $env:OFXGGML_AGENTS_TOOLS } else { "" }),
	[string]$HermesRoot = $(if ($env:HERMES_HOME) { $env:HERMES_HOME } elseif ($env:LOCALAPPDATA) { Join-Path $env:LOCALAPPDATA "hermes" } else { "" }),
	[switch]$Json,
	[switch]$Strict
)

$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$addonRoot = Resolve-Path (Join-Path $scriptRoot "..")
$addonsRoot = Split-Path -Parent $addonRoot
$script:Warnings = 0

function New-Check {
	param(
		[string]$State,
		[string]$Name,
		[string]$Detail = ""
	)
	if ($State -eq "WARN") {
		$script:Warnings++
	}
	return [pscustomobject]@{
		State = $State
		Name = $Name
		Detail = $Detail
	}
}

function Test-CommandAvailable {
	param([string]$Name)
	return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Test-PathCheck {
	param(
		[string]$Path,
		[string]$Name,
		[string]$MissingDetail,
		[switch]$Directory
	)
	$exists = if ($Directory) {
		Test-Path -LiteralPath $Path -PathType Container
	} else {
		Test-Path -LiteralPath $Path -PathType Leaf
	}
	if ($exists) {
		return New-Check "OK" $Name $Path
	}
	return New-Check "WARN" $Name $MissingDetail
}

function Test-ConfiguredFile {
	param(
		[string]$Path,
		[string]$Name,
		[string]$Hint
	)
	if ([string]::IsNullOrWhiteSpace($Path)) {
		return New-Check "WARN" $Name $Hint
	}
	$expanded = [Environment]::ExpandEnvironmentVariables($Path)
	if (Test-Path -LiteralPath $expanded -PathType Leaf) {
		return New-Check "OK" $Name $expanded
	}
	return New-Check "WARN" $Name "configured path was not found: $expanded"
}

function Test-ConfiguredDirectory {
	param(
		[string]$Path,
		[string]$Name,
		[string]$Hint
	)
	if ([string]::IsNullOrWhiteSpace($Path)) {
		return New-Check "WARN" $Name $Hint
	}
	$expanded = [Environment]::ExpandEnvironmentVariables($Path)
	if (Test-Path -LiteralPath $expanded -PathType Container) {
		return New-Check "OK" $Name $expanded
	}
	return New-Check "WARN" $Name "configured path was not found: $expanded"
}

function Test-ForbiddenPath {
	param([string]$RelativePath)
	$path = Join-Path $addonRoot $RelativePath
	if (Test-Path -LiteralPath $path) {
		return New-Check "WARN" "artifact hygiene" "generated/local path exists: $RelativePath"
	}
	return $null
}

$checks = @()
$checks += New-Check "OK" "addon root" $addonRoot.Path

foreach ($tool in @("git", "cmake")) {
	if (Test-CommandAvailable $tool) {
		$checks += New-Check "OK" $tool ((Get-Command $tool).Source)
	} else {
		$checks += New-Check "WARN" $tool "not found in PATH"
	}
}

$checks += Test-PathCheck `
	-Path (Join-Path $addonsRoot "ofxGgmlCore") `
	-Name "ofxGgmlCore sibling" `
	-MissingDetail "clone beside ofxGgmlAgents" `
	-Directory

$checks += Test-PathCheck `
	-Path (Join-Path $addonsRoot "ofxImGui") `
	-Name "ofxImGui" `
	-MissingDetail "install beside ofxGgmlAgents before building the planner example" `
	-Directory

$checks += Test-PathCheck `
	-Path (Join-Path $addonRoot "ofxGgmlAgentsPlannerExample\addons.make") `
	-Name "planner example" `
	-MissingDetail "ofxGgmlAgentsPlannerExample skeleton is missing"

$checks += Test-PathCheck `
	-Path (Join-Path $addonRoot "ofxGgmlAgentsPlannerExample\src\ofApp.cpp") `
	-Name "planner example source" `
	-MissingDetail "planner example source is missing"

$checks += Test-PathCheck `
	-Path (Join-Path $addonRoot "src\ofxGgmlAgents\ofxGgmlAgentsTypes.h") `
	-Name "agent request types" `
	-MissingDetail "agent request types header is missing"

$checks += Test-PathCheck `
	-Path (Join-Path $addonRoot "src\ofxGgmlAgents\ofxGgmlAgentsUtils.cpp") `
	-Name "agent utilities" `
	-MissingDetail "agent utility implementation is missing"

if (![string]::IsNullOrWhiteSpace($Goal)) {
	$checks += New-Check "OK" "agent goal" $Goal
} else {
	$checks += New-Check "WARN" "agent goal" "set OFXGGML_AGENTS_GOAL or pass -Goal"
}

if (![string]::IsNullOrWhiteSpace($Prompt)) {
	$checks += New-Check "OK" "agent prompt" "configured"
} else {
	$checks += New-Check "WARN" "agent prompt" "set OFXGGML_AGENTS_PROMPT or pass -Prompt"
}

$checks += Test-ConfiguredFile `
	-Path $Model `
	-Name "agent model" `
	-Hint "set OFXGGML_AGENTS_MODEL or pass -Model when a backend is available"

if (![string]::IsNullOrWhiteSpace($Tools)) {
	$toolCount = @($Tools -split ";" | Where-Object { ![string]::IsNullOrWhiteSpace($_) }).Count
	$checks += New-Check "OK" "agent tools" "$toolCount configured"
} else {
	$checks += New-Check "WARN" "agent tools" "set OFXGGML_AGENTS_TOOLS with semicolon-separated tool names when available"
}

$expandedHermesRoot = if ([string]::IsNullOrWhiteSpace($HermesRoot)) { "" } else { [Environment]::ExpandEnvironmentVariables($HermesRoot) }
$checks += Test-ConfiguredDirectory `
	-Path $expandedHermesRoot `
	-Name "Hermes root" `
	-Hint "set HERMES_HOME or install Hermes under LOCALAPPDATA\hermes"
if (![string]::IsNullOrWhiteSpace($expandedHermesRoot) -and (Test-Path -LiteralPath $expandedHermesRoot -PathType Container)) {
	$checks += Test-PathCheck `
		-Path (Join-Path $expandedHermesRoot "hermes-agent") `
		-Name "Hermes agent repo" `
		-MissingDetail "Hermes root exists but hermes-agent was not found" `
		-Directory
	$checks += Test-PathCheck `
		-Path (Join-Path $expandedHermesRoot "config.yaml") `
		-Name "Hermes config" `
		-MissingDetail "Hermes root exists but config.yaml was not found"
	$checks += Test-PathCheck `
		-Path (Join-Path $expandedHermesRoot "memories") `
		-Name "Hermes memory store" `
		-MissingDetail "Hermes root exists but memories directory was not found" `
		-Directory
}

$artifactWarnings = @()
foreach ($relative in @(
	"build",
	".vs",
	"ofxGgmlAgentsPlannerExample\bin",
	"ofxGgmlAgentsPlannerExample\obj",
	"ofxGgmlAgentsPlannerExample\.vs",
	"models"
)) {
	$warning = Test-ForbiddenPath -RelativePath $relative
	if ($null -ne $warning) {
		$artifactWarnings += $warning
	}
}
if ($artifactWarnings.Count -eq 0) {
	$checks += New-Check "OK" "artifact hygiene" "no generated/local paths detected"
} else {
	$checks += $artifactWarnings
}

if ($Json) {
	[pscustomobject]@{
		Root = $addonRoot.Path
		HermesRoot = $expandedHermesRoot
		Warnings = $script:Warnings
		Checks = $checks
	} | ConvertTo-Json -Depth 5
} else {
	Write-Host "ofxGgmlAgents doctor"
	Write-Host "Root  $addonRoot"
	Write-Host ""
	foreach ($check in $checks) {
		$line = "{0,-5} {1}" -f $check.State, $check.Name
		if (![string]::IsNullOrWhiteSpace($check.Detail)) {
			$line += " - $($check.Detail)"
		}
		Write-Host $line
	}
	Write-Host ""
	if ($script:Warnings -eq 0) {
		Write-Host "Doctor passed."
	} else {
		Write-Host "Doctor found $script:Warnings warning(s)."
	}
}

if ($Strict -and $script:Warnings -gt 0) {
	exit 1
}
