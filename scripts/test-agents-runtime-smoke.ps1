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

	Write-Step "Agents runtime smoke tool-call normalization"
	$ecosystemFixture = Join-Path ([System.IO.Path]::GetTempPath()) "ofxGgmlAgents-ecosystem-fixture.yaml"
	Set-Content -LiteralPath $ecosystemFixture -Value "development_order:`n  - lane: proven_lane`n    status: proven`n  - lane: planned_lane`n    status: planned"
	$fixtureCases = @(
		@{ Name = "structured-tool-calls"; First = @{ choices = @(@{ message = @{ role = "assistant"; content = ""; tool_calls = @(@{ id = "call_1"; type = "function"; function = @{ name = "get_proven_lanes"; arguments = "{}" } }) } }) } },
		@{ Name = "json-in-content"; First = @{ choices = @(@{ message = @{ role = "assistant"; content = '{"name":"get_proven_lanes","arguments":{}}' } }) } }
	)
	foreach ($fixtureCase in $fixtureCases) {
		$responseFixture = Join-Path ([System.IO.Path]::GetTempPath()) "ofxGgmlAgents-$($fixtureCase.Name)-fixture.json"
		@($fixtureCase.First, @{ choices = @(@{ message = @{ role = "assistant"; content = "OFXGGML_AGENTS_TOOL_OK" } }) }) | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $responseFixture
		$fixtureOutput = & $script -ServerBaseUrl "http://fixture.invalid" -Model "fixture-model" -EnableTools -RequireEndpoint -RequireToolExecution -EcosystemPath $ecosystemFixture -ResponseFixturePath $responseFixture -Json -SummaryOnly 2>&1 6>&1 | Out-String
		$fixtureSummary = $fixtureOutput | ConvertFrom-Json
		if (!$fixtureSummary.Summary.Passed -or !$fixtureSummary.Summary.ToolExecutionBacked) { throw "$($fixtureCase.Name) fixture did not prove tool execution.`n$fixtureOutput" }
		if ($fixtureSummary.Summary.ToolCallEncoding -ne $fixtureCase.Name) { throw "Unexpected tool encoding: $($fixtureSummary.Summary.ToolCallEncoding)" }
		Remove-Item -LiteralPath $responseFixture -Force
	}
	Remove-Item -LiteralPath $ecosystemFixture -Force

	Write-Step "Agents runtime smoke contract passed"
} finally {
	Restore-Environment
}
