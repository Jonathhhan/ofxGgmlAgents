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

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$script = Join-Path $scriptRoot "run-agents-runtime-smoke.ps1"

$originalServerUrl = $env:OFXGGML_AGENT_LLM_BASE_URL
$originalModel = $env:OFXGGML_AGENT_LLM_MODEL
$originalApiKey = $env:OFXGGML_AGENT_LLM_API_KEY

function Restore-Environment {
	$env:OFXGGML_AGENT_LLM_BASE_URL = $originalServerUrl
	$env:OFXGGML_AGENT_LLM_MODEL = $originalModel
	$env:OFXGGML_AGENT_LLM_API_KEY = $originalApiKey
}

try {
	$env:OFXGGML_AGENT_LLM_BASE_URL = ""
	$env:OFXGGML_AGENT_LLM_MODEL = ""
	$env:OFXGGML_AGENT_LLM_API_KEY = ""

	Write-Step "Agents runtime smoke dry-run"
	$textOutput = & $script -DryRun 2>&1 6>&1 | Out-String
	Assert-Contains $textOutput "ofxGgmlAgents runtime smoke plan" "runtime smoke dry-run"
	Assert-Contains $textOutput "Backend: planning-boundary" "runtime smoke dry-run"
	Assert-Contains $textOutput "HermesInstalled:" "runtime smoke dry-run"
	Assert-Contains $textOutput "ModelBacked: False" "runtime smoke dry-run"
	Assert-Contains $textOutput "EndpointConfigured: False" "runtime smoke dry-run"
	Assert-Contains $textOutput "ToolExecutionBacked: False" "runtime smoke dry-run"
	Assert-Contains $textOutput "Dry run complete; no files were changed" "runtime smoke dry-run"

	Write-Step "Agents runtime smoke JSON dry-run"
	$jsonOutput = & $script -DryRun -Json -SummaryOnly 2>&1 6>&1 | Out-String
	$summary = $jsonOutput | ConvertFrom-Json
	if ($summary.Name -ne "ofxGgmlAgents runtime smoke") {
		throw "Unexpected runtime smoke name: $($summary.Name)"
	}
	if ($summary.Backend -ne "planning-boundary") {
		throw "Unexpected runtime smoke backend: $($summary.Backend)"
	}
	if ($summary.ModelBacked -or $summary.ToolExecutionBacked) {
		throw "Agents runtime smoke should not claim model-backed or tool-execution-backed runtime yet."
	}
	if ($null -eq $summary.HermesInstalled) {
		throw "Runtime dry-run summary did not include HermesInstalled."
	}
	if ($summary.ModelPath) {
		throw "Runtime dry-run summary should not include model path when endpoint is not configured."
	}
	if (!($summary.NextCommands -contains "scripts\run-agents-runtime-smoke.bat -Json -SummaryOnly")) {
		throw "JSON dry-run did not include the runtime smoke command."
	}
	if (!($summary.NextCommands -contains "scripts\run-agents-runtime-smoke.bat -Json -SummaryOnly -OutputPath .agents-runtime-smoke.json")) {
		throw "JSON dry-run did not include the runnable evidence command."
	}

	Write-Step "Agents runtime smoke contract passed"
} finally {
	Restore-Environment
}
