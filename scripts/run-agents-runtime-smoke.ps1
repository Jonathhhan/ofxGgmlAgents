param(
	[string]$Configuration = "Release",
	[string]$BuildDir = "",
	[string]$ServerBaseUrl = $(if ($env:OFXGGML_AGENT_LLM_BASE_URL) { $env:OFXGGML_AGENT_LLM_BASE_URL } else { "" }),
	[string]$Model = $(if ($env:OFXGGML_AGENT_LLM_MODEL) { $env:OFXGGML_AGENT_LLM_MODEL } else { "" }),
	[string]$Prompt = "Reply with exactly OFXGGML_AGENTS_SMOKE_OK",
	[int]$TimeoutSeconds = 30,
	[string]$ApiKey = $(if ($env:OFXGGML_AGENT_LLM_API_KEY) { $env:OFXGGML_AGENT_LLM_API_KEY } else { "" }),
	[string]$OutputPath = "",
	[switch]$Clean,
	[switch]$DryRun,
	[switch]$Json,
	[switch]$SummaryOnly,
	[switch]$RequireEndpoint
)

$ErrorActionPreference = "Stop"

function Write-Step {
	param([string]$Message)
	if (!$Json) {
		Write-Host "==> $Message"
	}
}

function Get-PowerShellExecutable {
	$pwsh = Get-Command pwsh -ErrorAction SilentlyContinue
	if ($pwsh) {
		return $pwsh.Source
	}

	$windowsPowerShell = Get-Command powershell -ErrorAction SilentlyContinue
	if ($windowsPowerShell) {
		return $windowsPowerShell.Source
	}

	throw "Could not find pwsh or powershell."
}

function Write-SmokeOutputPath {
	param(
		[string]$Path,
		[string]$Content
	)
	if ([string]::IsNullOrWhiteSpace($Path)) {
		return
	}
	$target = if ([System.IO.Path]::IsPathRooted($Path)) {
		$Path
	} else {
		Join-Path $addonRoot $Path
	}
	$directory = Split-Path -Parent $target
	if (!(Test-Path -LiteralPath $directory -PathType Container)) {
		New-Item -ItemType Directory -Path $directory -Force | Out-Null
	}
	Set-Content -LiteralPath $target -Value $Content
}

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$addonRoot = Resolve-Path (Join-Path $scriptRoot "..")
$addonsRoot = Split-Path -Parent $addonRoot
$testScript = Join-Path $scriptRoot "test-addon.ps1"
$doctorScript = Join-Path $scriptRoot "doctor-agents.ps1"

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
	$BuildDir = Join-Path ([System.IO.Path]::GetTempPath()) "ofxGgmlAgents-runtime-smoke"
}

function Test-RuntimeSmokeReady {
	return (Test-Path -LiteralPath (Join-Path $addonRoot "src\ofxGgmlAgents\ofxGgmlAgentsTypes.h") -PathType Leaf) -and
		(Test-Path -LiteralPath (Join-Path $addonRoot "src\ofxGgmlAgents\ofxGgmlAgentsUtils.cpp") -PathType Leaf) -and
		(Test-Path -LiteralPath (Join-Path $addonRoot "tests\test_main.cpp") -PathType Leaf) -and
		(Test-Path -LiteralPath (Join-Path $addonsRoot "ofxGgmlCore") -PathType Container)
}

function New-DryRunSummary {
	$ready = Test-RuntimeSmokeReady
	$endpointConfigured = !([string]::IsNullOrWhiteSpace($ServerBaseUrl) -or [string]::IsNullOrWhiteSpace($Model))

	return [ordered]@{
		Name = "ofxGgmlAgents runtime smoke"
		Root = [string]$addonRoot
		Backend = "planning-boundary"
		BuildDir = $BuildDir
		Ready = $ready
		ModelBacked = [bool]$endpointConfigured
		ToolExecutionBacked = $false
		EndpointConfigured = [bool]$endpointConfigured
		TestScript = $testScript
		DoctorScript = $doctorScript
		NextCommands = @(
			"scripts\run-agents-runtime-smoke.bat -Json -SummaryOnly",
			"scripts\run-agents-runtime-smoke.bat -Json -SummaryOnly -OutputPath .agents-runtime-smoke.json",
			"scripts\test-addon.bat",
			"scripts\doctor-agents.bat"
		)
	}
}

function Invoke-SmokeStep {
	param(
		[string]$Name,
		[string[]]$Arguments
	)

	$output = @()
	$exitCode = 0
	try {
		$output = & $powerShell @Arguments 2>&1 | ForEach-Object { "$_" }
		$exitCode = $LASTEXITCODE
	} catch {
		$output += "$_"
		$exitCode = 1
	}

	return [ordered]@{
		Name = $Name
		Passed = ($exitCode -eq 0)
		ExitCode = $exitCode
		Output = $output
	}
}

function Resolve-AgentEndpoint {
	param([string]$BaseUrl)

	$trimmed = if ($null -eq $BaseUrl) { "" } else { $BaseUrl.Trim() }
	if ([string]::IsNullOrWhiteSpace($trimmed)) {
		return ""
	}
	$normalized = $trimmed.TrimEnd("/")
	if ($normalized -match "(?i)/v1$") {
		return "$normalized/chat/completions"
	}
	return "$normalized/v1/chat/completions"
}

function Invoke-AgentEndpointSmoke {
	param(
		[string]$BaseUrl,
		[string]$Model,
		[string]$Prompt,
		[string]$ApiKey,
		[int]$TimeoutSeconds
	)

	$endpoint = Resolve-AgentEndpoint -BaseUrl $BaseUrl
	if ([string]::IsNullOrWhiteSpace($endpoint)) {
		return [ordered]@{
			Passed = $false
			ExitCode = 2
			Error = "agent model endpoint was not configured"
			SmokeKind = "openai-compatible-chat"
			Backend = "openai-compatible"
			ModelPath = "<not-configured>"
			ElapsedMs = 0
			ResponseText = ""
		}
	}
	if ([string]::IsNullOrWhiteSpace($Model)) {
		return [ordered]@{
			Passed = $false
			ExitCode = 2
			Error = "agent model alias is required"
			SmokeKind = "openai-compatible-chat"
			Backend = "openai-compatible"
			ModelPath = "<not-configured>"
			ElapsedMs = 0
			ResponseText = ""
		}
	}

	$payload = [ordered]@{
		model = $Model
		messages = @(
			[ordered]@{
				role = "system"
				content = "Reply with exactly OFXGGML_AGENTS_SMOKE_OK and no extra text."
			},
			[ordered]@{
				role = "user"
				content = $Prompt
			}
		)
		temperature = 0
		max_tokens = 32
		stream = $false
	}

	$headers = @{}
	if (![string]::IsNullOrWhiteSpace($ApiKey)) {
		$headers["Authorization"] = "Bearer $ApiKey"
	}
	$started = Get-Date
	try {
		$response = Invoke-RestMethod `
			-Method Post `
			-Uri $endpoint `
			-Headers $headers `
			-Body (ConvertTo-Json $payload -Depth 10) `
			-ContentType "application/json" `
			-TimeoutSec ([Math]::Max(1, $TimeoutSeconds))
		$elapsedMs = [int]((Get-Date) - $started).TotalMilliseconds
	} catch {
		$elapsedMs = [int]((Get-Date) - $started).TotalMilliseconds
		return [ordered]@{
			Passed = $false
			ExitCode = 1
			Error = $_.Exception.Message
			SmokeKind = "openai-compatible-chat"
			Backend = "openai-compatible"
			ModelPath = $Model
			ElapsedMs = $elapsedMs
			ResponseText = ""
		}
	}

	$reply = ""
	if ($response.choices -is [array] -and $response.choices.Count -gt 0) {
		$choice = $response.choices[0]
		if ($choice.message -and $choice.message.PSObject.Properties["content"]) {
			$reply = [string]$choice.message.content
		} elseif ($choice.text -ne $null) {
			$reply = [string]$choice.text
		}
	}
	$replyText = $reply.Trim()
	$expected = "OFXGGML_AGENTS_SMOKE_OK"
	return [ordered]@{
		Passed = ($replyText -eq $expected)
		ExitCode = if ($replyText -eq $expected) { 0 } else { 3 }
		Error = if ($replyText -eq $expected) { "" } else { "agent endpoint smoke did not return OFXGGML_AGENTS_SMOKE_OK" }
		SmokeKind = "openai-compatible-chat"
		Backend = "openai-compatible"
		ModelPath = $Model
		ElapsedMs = $elapsedMs
		ResponseText = $replyText
	}
}

if ($DryRun) {
	$summary = New-DryRunSummary
	if ($Json) {
		if ($SummaryOnly) {
			$summary | ConvertTo-Json -Depth 5
		} else {
			[ordered]@{
				Summary = $summary
				Results = @()
			} | ConvertTo-Json -Depth 6
		}
		return
	}

	Write-Step "ofxGgmlAgents runtime smoke plan"
	Write-Host "  Backend: $($summary.Backend)"
	Write-Host "  BuildDir: $($summary.BuildDir)"
	Write-Host "  Ready: $($summary.Ready)"
	Write-Host "  ModelBacked: $($summary.ModelBacked)"
	Write-Host "  ToolExecutionBacked: $($summary.ToolExecutionBacked)"
	Write-Host "  EndpointConfigured: $($summary.EndpointConfigured)"
	Write-Host "  Test: $($summary.TestScript)"
	Write-Host "  Doctor: $($summary.DoctorScript)"
	Write-Host "  Next: $($summary.NextCommands[0])"
	Write-Step "Dry run complete; no files were changed"
	return
}

$started = Get-Date
$powerShell = Get-PowerShellExecutable
$testArgs = @(
	"-NoProfile",
	"-ExecutionPolicy",
	"Bypass",
	"-File",
	$testScript,
	"-Configuration",
	$Configuration,
	"-BuildDir",
	$BuildDir
)
if ($Clean) {
	$testArgs += "-Clean"
}
$doctorArgs = @(
	"-NoProfile",
	"-ExecutionPolicy",
	"Bypass",
	"-File",
	$doctorScript,
	"-Json"
)

$results = @()
$results += Invoke-SmokeStep -Name "planning helper tests" -Arguments $testArgs
$results += Invoke-SmokeStep -Name "Agents doctor" -Arguments $doctorArgs

$endpointConfigured = !([string]::IsNullOrWhiteSpace($ServerBaseUrl) -or [string]::IsNullOrWhiteSpace($Model))
$endpointSummary = $null
if ($endpointConfigured -or $RequireEndpoint) {
	$endpointSummary = Invoke-AgentEndpointSmoke -BaseUrl $ServerBaseUrl -Model $Model -Prompt $Prompt -ApiKey $ApiKey -TimeoutSeconds $TimeoutSeconds
	$results += [ordered]@{
		Name = "agent endpoint smoke"
		Passed = [bool]$endpointSummary.Passed
		ExitCode = [int]$endpointSummary.ExitCode
		Output = @($endpointSummary.Error, $endpointSummary.ResponseText)
	}
}

$failed = @($results | Where-Object { -not $_.Passed })
$elapsedMs = [int]((Get-Date) - $started).TotalMilliseconds

$modelBacked = $false
$smokeKind = "planning-boundary"
$backend = "planning-boundary"
$modelPath = "<not-configured>"
$inferenceChecked = $false
if ($endpointSummary) {
	$smokeKind = [string]$endpointSummary.SmokeKind
	$backend = [string]$endpointSummary.Backend
	$modelBacked = [bool]$endpointSummary.Passed
	$inferenceChecked = [bool]$endpointSummary.Passed
	$modelPath = [string]$endpointSummary.ModelPath
	if ([string]::IsNullOrWhiteSpace($modelPath)) {
		$modelPath = "<not-configured>"
	}
}

$summary = [ordered]@{
	Name = "ofxGgmlAgents runtime smoke"
	Passed = ($failed.Count -eq 0)
	Backend = $backend
	Configuration = $Configuration
	BuildDir = $BuildDir
	ModelBacked = [bool]$modelBacked
	ToolExecutionBacked = $false
	InferenceChecked = [bool]$inferenceChecked
	SmokeKind = $smokeKind
	ModelPath = [string]$modelPath
	ResultCount = $results.Count
	FailedCount = $failed.Count
	ElapsedMs = $elapsedMs
	Error = $(if ($failed.Count -eq 0) { "" } else { (($failed | ForEach-Object { $_.Output }) -join "`n") })
}

if ($Json) {
	if ($SummaryOnly) {
		$payload = [ordered]@{
			Name = [string]$summary.Name
			Summary = @{
				Passed = [bool]$summary.Passed
				InferenceChecked = [bool]$summary.InferenceChecked
				SmokeKind = [string]$summary.SmokeKind
				Backend = [string]$summary.Backend
				ModelPath = [string]$summary.ModelPath
			}
			Error = [string]$summary.Error
			NextCommands = @(
				"scripts\run-agents-runtime-smoke.bat -Json -SummaryOnly",
				"scripts\run-agents-runtime-smoke.bat -Json -SummaryOnly -OutputPath .agents-runtime-smoke.json",
				"scripts\test-addon.bat",
				"scripts\doctor-agents.bat"
			)
		}
		$content = ConvertTo-Json $payload -Depth 6
		Write-SmokeOutputPath -Path $OutputPath -Content $content
		$content
	} else {
		$payload = [ordered]@{
			Summary = $summary
			Results = $results
		}
		$content = ConvertTo-Json $payload -Depth 6
		Write-SmokeOutputPath -Path $OutputPath -Content $content
		$content
	}
} else {
	foreach ($result in $results) {
		Write-Step $result.Name
		foreach ($line in $result.Output) {
			Write-Host $line
		}
	}
	Write-Step "ofxGgmlAgents runtime smoke summary"
	Write-Host "  Backend: $($summary.Backend)"
	Write-Host "  ModelBacked: $($summary.ModelBacked)"
	Write-Host "  InferenceChecked: $($summary.InferenceChecked)"
	Write-Host "  SmokeKind: $($summary.SmokeKind)"
	Write-Host "  ModelPath: $($summary.ModelPath)"
	Write-Host "  ToolExecutionBacked: $($summary.ToolExecutionBacked)"
	Write-Host "  Passed: $($summary.Passed)"
	Write-Host "  ElapsedMs: $($summary.ElapsedMs)"
}

if ($failed.Count -gt 0) {
	exit 1
}
