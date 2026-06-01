param()

$ErrorActionPreference = "Stop"

function Write-Step {
	param([string]$Message)
	Write-Host "==> $Message"
}

function Assert-Contains {
	param(
		[string]$Text,
		[string]$Needle,
		[string]$Label
	)
	if (!$Text.Contains($Needle)) {
		throw "$Label did not contain expected text: $Needle`n$Text"
	}
}

function Assert-NotContains {
	param(
		[string]$Text,
		[string]$Needle,
		[string]$Label
	)
	if ($Text.Contains($Needle)) {
		throw "$Label contained unexpected text: $Needle`n$Text"
	}
}

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$script = Join-Path $scriptRoot "generate-release-readiness-score.ps1"

Write-Step "Release readiness JSON summary"
$summaryJson = & $script -Json -SummaryOnly 2>&1 | Out-String
$summary = $summaryJson | ConvertFrom-Json
if ($summary.Addon -ne "ofxGgmlAgents") {
	throw "Unexpected release readiness addon: $($summary.Addon)"
}
if ($summary.Score -ne 35 -or $summary.MaxScore -ne 35) {
	throw "Unexpected release readiness score: $($summary.Score)/$($summary.MaxScore)"
}
if ($summary.PSObject.Properties.Name -contains "Checks") {
	throw "Summary JSON should not include Checks."
}

Write-Step "Release readiness JSON detail"
$detailJson = & $script -Json 2>&1 | Out-String
$detail = $detailJson | ConvertFrom-Json
if (!$detail.Checks -or $detail.Checks.Count -eq 0) {
	throw "Detailed JSON should include Checks."
}
if (!($detail.Checks.Name -contains "ofxGgmlAgentsPlannerExample README")) {
	throw "Detailed JSON did not include the planner example README check."
}
if (!($detail.Checks.Name -contains "ofxGgmlAgentsCodexLocalExample handoff README")) {
	throw "Detailed JSON did not include the Codex handoff README check."
}

Write-Step "Release readiness text summary"
$summaryText = & $script -SummaryOnly 2>&1 6>&1 | Out-String
Assert-Contains $summaryText "ofxGgmlAgents release readiness score" "release readiness text summary"
Assert-Contains $summaryText "Score:      35/35" "release readiness text summary"
Assert-NotContains $summaryText "[PASS]" "release readiness text summary"

Write-Step "Release readiness contract passed"
