param(
	[string]$Configuration = "Release",
	[string]$BuildDir = "",
	[string]$ServerBaseUrl = $(if ($env:OFXGGML_AGENT_LLM_BASE_URL) { $env:OFXGGML_AGENT_LLM_BASE_URL } else { "" }),
	[string]$Model = $(if ($env:OFXGGML_AGENT_LLM_MODEL) { $env:OFXGGML_AGENT_LLM_MODEL } else { "" }),
	[string]$Prompt = "Reply with exactly OFXGGML_AGENTS_SMOKE_OK",
	[int]$TimeoutSeconds = 30,
	[string]$ApiKey = $(if ($env:OFXGGML_AGENT_LLM_API_KEY) { $env:OFXGGML_AGENT_LLM_API_KEY } else { "" }),
	[string]$HermesRoot = $(if ($env:HERMES_HOME) { $env:HERMES_HOME } elseif ($env:LOCALAPPDATA) { Join-Path $env:LOCALAPPDATA "hermes" } else { "" }),
	[string]$OutputPath = "",
	[string]$EcosystemPath = $(if ($env:OFXGGML_AGENT_ECOSYSTEM_PATH) { $env:OFXGGML_AGENT_ECOSYSTEM_PATH } else { "" }),
	[string]$ResponseFixturePath = "",
	[switch]$Clean,
	[switch]$DryRun,
	[switch]$Json,
	[switch]$SummaryOnly,
	[switch]$RequireEndpoint,
	[switch]$EnableTools,
	[switch]$RequireToolExecution
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

function Test-GeneratedBuildDir {
	param([string]$Path)
	if ([string]::IsNullOrWhiteSpace($Path)) {
		return $false
	}
	$fullPath = [System.IO.Path]::GetFullPath($Path)
	$tempRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath()).TrimEnd("\", "/") + [System.IO.Path]::DirectorySeparatorChar
	$addonBuildRoot = [System.IO.Path]::GetFullPath((Join-Path $addonRoot.Path "build")).TrimEnd("\", "/") + [System.IO.Path]::DirectorySeparatorChar
	return $fullPath.StartsWith($tempRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
		$fullPath.StartsWith($addonBuildRoot, [System.StringComparison]::OrdinalIgnoreCase)
}

function Get-CachedCxxCompiler {
	param([string]$BuildDir)
	$cachePath = Join-Path $BuildDir "CMakeCache.txt"
	if (!(Test-Path -LiteralPath $cachePath -PathType Leaf)) {
		return ""
	}
	foreach ($line in (Get-Content -LiteralPath $cachePath -ErrorAction SilentlyContinue)) {
		if ($line -match "^CMAKE_CXX_COMPILER:FILEPATH=(.+)$") {
			return $Matches[1]
		}
	}
	return ""
}

function Clear-StaleCMakeBuildDir {
	param([string]$BuildDir)
	if (!(Test-GeneratedBuildDir -Path $BuildDir)) {
		return
	}
	$compiler = Get-CachedCxxCompiler -BuildDir $BuildDir
	if (![string]::IsNullOrWhiteSpace($compiler) -and !(Test-Path -LiteralPath $compiler -PathType Leaf)) {
		Write-Step "Cleaning stale CMake cache in $BuildDir"
		Remove-Item -LiteralPath $BuildDir -Recurse -Force
	}
}

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$addonRoot = Resolve-Path (Join-Path $scriptRoot "..")
$addonsRoot = Split-Path -Parent $addonRoot
$testScript = Join-Path $scriptRoot "test-addon.ps1"
$doctorScript = Join-Path $scriptRoot "doctor-agents.ps1"

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
	$BuildDir = Join-Path ([System.IO.Path]::GetTempPath()) "ofxGgmlAgents-runtime-smoke"
}
Clear-StaleCMakeBuildDir -BuildDir $BuildDir

function Test-RuntimeSmokeReady {
	return (Test-Path -LiteralPath (Join-Path $addonRoot "src\ofxGgmlAgents\ofxGgmlAgentsTypes.h") -PathType Leaf) -and
		(Test-Path -LiteralPath (Join-Path $addonRoot "src\ofxGgmlAgents\ofxGgmlAgentsUtils.cpp") -PathType Leaf) -and
		(Test-Path -LiteralPath (Join-Path $addonRoot "tests\test_main.cpp") -PathType Leaf)
}

function New-DryRunSummary {
	$ready = Test-RuntimeSmokeReady
	$endpointConfigured = !([string]::IsNullOrWhiteSpace($ServerBaseUrl) -or [string]::IsNullOrWhiteSpace($Model))
	$expandedHermesRoot = if ([string]::IsNullOrWhiteSpace($HermesRoot)) { "" } else { [Environment]::ExpandEnvironmentVariables($HermesRoot) }
	$hermesInstalled = ![string]::IsNullOrWhiteSpace($expandedHermesRoot) -and
		(Test-Path -LiteralPath $expandedHermesRoot -PathType Container) -and
		(Test-Path -LiteralPath (Join-Path $expandedHermesRoot "hermes-agent") -PathType Container)

	return [ordered]@{
		Name = "ofxGgmlAgents runtime smoke"
		Root = [string]$addonRoot
		Backend = "planning-boundary"
		BuildDir = $BuildDir
		Ready = $ready
		HermesInstalled = [bool]$hermesInstalled
		HermesRoot = $expandedHermesRoot
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

function Resolve-EcosystemPath {
	param([string]$ConfiguredPath)
	if (![string]::IsNullOrWhiteSpace($ConfiguredPath)) { return [System.IO.Path]::GetFullPath($ConfiguredPath) }
	$dotGitPath = Join-Path $addonRoot ".git"
	if (Test-Path -LiteralPath $dotGitPath -PathType Leaf) {
		$dotGit = (Get-Content -LiteralPath $dotGitPath -Raw).Trim()
		if ($dotGit -match "^gitdir:\s*(.+)$") {
			$gitDir = [System.IO.Path]::GetFullPath($Matches[1])
			$canonicalAddonRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $gitDir))
			return Join-Path (Split-Path -Parent $canonicalAddonRoot) "ofxGgmlWorkflows\ecosystem.yaml"
		}
	}
	return Join-Path $addonsRoot "ofxGgmlWorkflows\ecosystem.yaml"
}

function Get-CapabilityStatuses {
	param([string]$Path)
	if (!(Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Canonical ecosystem file was not found: $Path" }
	$capabilities = @()
	$current = [ordered]@{}
	$inDevelopmentOrder = $false
	foreach ($line in (Get-Content -LiteralPath $Path)) {
		if ($line -match '^development_order:\s*$') { $inDevelopmentOrder = $true; continue }
		if (!$inDevelopmentOrder) { continue }
		if ($line -match '^\S' -and $line -notmatch '^development_order:') { break }
		if ($line -match '^\s+-\s+order:\s*\d+\s*$') {
			if ($current.lane -and $current.status) { $capabilities += [pscustomobject]$current }
			$current = [ordered]@{}
		} elseif ($line -match '^\s+lane:\s*(\S+)\s*$') { $current.lane = $Matches[1] }
		elseif ($line -match '^\s+capability:\s*(\S+)\s*$') { $current.capability = $Matches[1] }
		elseif ($line -match '^\s+status:\s*(\S+)\s*$') { $current.status = $Matches[1] }
		elseif ($line -match '^\s+proof:\s*(\S+)\s*$') { $current.proof = $Matches[1] }
	}
	if ($current.lane -and $current.status) { $capabilities += [pscustomobject]$current }
	return @($capabilities)
}

function Get-NormalizedToolCall {
	param($Message)
	if ($Message.tool_calls -is [array] -and $Message.tool_calls.Count -gt 0) {
		$call = $Message.tool_calls[0]
		return [ordered]@{ Id = [string]$call.id; Name = [string]$call.function.name; Arguments = [string]$call.function.arguments; Encoding = "structured-tool-calls" }
	}
	try {
		$decoded = ([string]$Message.content).Trim() | ConvertFrom-Json -ErrorAction Stop
		if ($decoded.name) {
			return [ordered]@{ Id = ""; Name = [string]$decoded.name; Arguments = ($decoded.arguments | ConvertTo-Json -Compress -Depth 10); Encoding = "json-in-content" }
		}
	} catch {}
	$content = [string]$Message.content
	if ($content -match '(?s)<function>\s*<name>\s*([^<]+?)\s*</name>\s*<arguments>\s*(\{.*?\})\s*</arguments>\s*</function>') {
		return [ordered]@{ Id = ""; Name = $Matches[1].Trim(); Arguments = $Matches[2].Trim(); Encoding = "xml-in-content" }
	}
	return $null
}

function Invoke-AgentEndpointSmoke {
	param([string]$BaseUrl, [string]$Model, [string]$Prompt, [string]$ApiKey, [int]$TimeoutSeconds,
		[bool]$ToolsEnabled, [string]$EcosystemPath, [string]$ResponseFixturePath)

	$endpoint = Resolve-AgentEndpoint -BaseUrl $BaseUrl
	if ([string]::IsNullOrWhiteSpace($endpoint) -or [string]::IsNullOrWhiteSpace($Model)) {
		return [ordered]@{ Passed = $false; ExitCode = 2; Error = "agent endpoint and model alias are required"; SmokeKind = "openai-compatible-chat"; Backend = "openai-compatible"; ModelPath = "<not-configured>"; ElapsedMs = 0; ResponseText = ""; ToolExecutionBacked = $false }
	}
	$messages = @(
		[ordered]@{ role = "system"; content = $(if ($ToolsEnabled) { "You must call get_capability_status with no arguments. After its result, reply with exactly OFXGGML_AGENTS_TOOL_OK and no extra text." } else { "Reply with exactly OFXGGML_AGENTS_SMOKE_OK and no extra text." }) },
		[ordered]@{ role = "user"; content = $(if ($ToolsEnabled) { "Call get_capability_status now." } else { $Prompt }) }
	)
	$payload = [ordered]@{ model = $Model; messages = $messages; temperature = 0; max_tokens = 128; stream = $false }
	if ($ToolsEnabled) {
		$payload.tools = @([ordered]@{ type = "function"; function = [ordered]@{ name = "get_capability_status"; description = "Read each lane's capability, current status, and proof identifier from the canonical ecosystem manifest."; parameters = [ordered]@{ type = "object"; properties = [ordered]@{}; additionalProperties = $false } } })
		$payload.tool_choice = "required"
	}
	$headers = @{}
	if ($ApiKey) { $headers["Authorization"] = "Bearer $ApiKey" }
	$fixtures = if ($ResponseFixturePath) { @(Get-Content -LiteralPath $ResponseFixturePath -Raw | ConvertFrom-Json) } else { $null }
	$requestIndex = 0
	$started = Get-Date
	try {
		$response = if ($fixtures) { $fixtures[$requestIndex++] } else { Invoke-RestMethod -Method Post -Uri $endpoint -Headers $headers -Body (ConvertTo-Json $payload -Depth 20) -ContentType "application/json" -TimeoutSec ([Math]::Max(1, $TimeoutSeconds)) }
		$choice = $response.choices[0]
		$message = $choice.message
		$reply = if ($message -and $message.PSObject.Properties["content"]) { [string]$message.content } else { [string]$choice.text }
		$toolExecuted = $false
		$toolName = ""
		$toolEncoding = ""
		if ($ToolsEnabled) {
			$toolCall = Get-NormalizedToolCall -Message $message
			if (!$toolCall) { throw "Model did not request an allowlisted tool" }
			$toolName = $toolCall.Name
			$toolEncoding = $toolCall.Encoding
			if ($toolName -ne "get_capability_status") { throw "Model requested non-allowlisted tool: $toolName" }
			$arguments = $toolCall.Arguments | ConvertFrom-Json
			if (@($arguments.PSObject.Properties).Count -ne 0) { throw "get_capability_status does not accept arguments" }
			$capabilities = @(Get-CapabilityStatuses -Path $EcosystemPath)
			if ($capabilities.Count -eq 0) { throw "Canonical ecosystem manifest contains no development-order capability statuses" }
			$toolResult = [ordered]@{ capabilities = $capabilities } | ConvertTo-Json -Compress -Depth 5
			$payload.messages = @($messages) + @($message)
			$toolMessage = [ordered]@{ role = "tool"; name = $toolName; content = $toolResult }
			if ($toolCall.Id) { $toolMessage.tool_call_id = $toolCall.Id }
			$payload.messages += $toolMessage
			[void]$payload.Remove("tool_choice")
			[void]$payload.Remove("tools")
			$response = if ($fixtures) { $fixtures[$requestIndex++] } else { Invoke-RestMethod -Method Post -Uri $endpoint -Headers $headers -Body (ConvertTo-Json $payload -Depth 20) -ContentType "application/json" -TimeoutSec ([Math]::Max(1, $TimeoutSeconds)) }
			$reply = [string]$response.choices[0].message.content
			$toolExecuted = $true
		}
		$elapsedMs = [int]((Get-Date) - $started).TotalMilliseconds
	} catch {
		return [ordered]@{ Passed = $false; ExitCode = 1; Error = $_.Exception.Message; SmokeKind = $(if ($ToolsEnabled) { "openai-compatible-tool-loop" } else { "openai-compatible-chat" }); Backend = "openai-compatible"; ModelPath = $Model; ElapsedMs = [int]((Get-Date) - $started).TotalMilliseconds; ResponseText = ""; ToolExecutionBacked = $false }
	}
	$replyText = $reply.Trim()
	$expected = if ($ToolsEnabled) { "OFXGGML_AGENTS_TOOL_OK" } else { "OFXGGML_AGENTS_SMOKE_OK" }
	$passed = $replyText -eq $expected -and (!$ToolsEnabled -or $toolExecuted)
	return [ordered]@{ Passed = $passed; ExitCode = $(if ($passed) { 0 } else { 3 }); Error = $(if ($passed) { "" } else { "agent endpoint smoke did not complete the expected model/tool loop" }); SmokeKind = $(if ($ToolsEnabled) { "openai-compatible-tool-loop" } else { "openai-compatible-chat" }); Backend = $(if ($fixtures) { "response-fixture" } else { "openai-compatible" }); ModelPath = $Model; ElapsedMs = $elapsedMs; ResponseText = $replyText; ToolExecutionBacked = [bool]$toolExecuted; ToolName = $toolName; ToolCallEncoding = $toolEncoding; FixtureBacked = [bool]$fixtures }
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
	Write-Host "  HermesInstalled: $($summary.HermesInstalled)"
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
if (![string]::IsNullOrWhiteSpace($HermesRoot)) {
	$doctorArgs += "-HermesRoot"
	$doctorArgs += $HermesRoot
}

$results = @()
$results += Invoke-SmokeStep -Name "planning helper tests" -Arguments $testArgs
$results += Invoke-SmokeStep -Name "Agents doctor" -Arguments $doctorArgs

$endpointConfigured = !([string]::IsNullOrWhiteSpace($ServerBaseUrl) -or [string]::IsNullOrWhiteSpace($Model))
$resolvedEcosystemPath = Resolve-EcosystemPath -ConfiguredPath $EcosystemPath
$endpointSummary = $null
if ($endpointConfigured -or $RequireEndpoint -or $ResponseFixturePath) {
	$endpointSummary = Invoke-AgentEndpointSmoke -BaseUrl $ServerBaseUrl -Model $Model -Prompt $Prompt -ApiKey $ApiKey -TimeoutSeconds $TimeoutSeconds -ToolsEnabled ([bool]$EnableTools) -EcosystemPath $resolvedEcosystemPath -ResponseFixturePath $ResponseFixturePath
	$results += [ordered]@{
		Name = "agent endpoint smoke"
		Passed = [bool]$endpointSummary.Passed
		ExitCode = [int]$endpointSummary.ExitCode
		Output = @($endpointSummary.Error, $endpointSummary.ResponseText)
	}
}
if ($RequireToolExecution -and (!$endpointSummary -or !$endpointSummary.ToolExecutionBacked)) {
	$results += [ordered]@{ Name = "required tool execution"; Passed = $false; ExitCode = 4; Output = @("required allowlisted tool execution was not demonstrated") }
}

$failed = @($results | Where-Object { -not $_.Passed })
$elapsedMs = [int]((Get-Date) - $started).TotalMilliseconds

$modelBacked = $false
$smokeKind = "planning-boundary"
$backend = "planning-boundary"
$modelPath = "<not-configured>"
$inferenceChecked = $false
$toolExecutionBacked = $false
if ($endpointSummary) {
	$smokeKind = [string]$endpointSummary.SmokeKind
	$backend = [string]$endpointSummary.Backend
	$modelBacked = [bool]$endpointSummary.Passed -and !$endpointSummary.FixtureBacked
	$inferenceChecked = [bool]$endpointSummary.Passed -and !$endpointSummary.FixtureBacked
	$modelPath = [string]$endpointSummary.ModelPath
	$toolExecutionBacked = [bool]$endpointSummary.ToolExecutionBacked
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
	ToolExecutionBacked = [bool]$toolExecutionBacked
	InferenceChecked = [bool]$inferenceChecked
	HermesInstalled = ![string]::IsNullOrWhiteSpace($HermesRoot) -and
		(Test-Path -LiteralPath ([Environment]::ExpandEnvironmentVariables($HermesRoot)) -PathType Container) -and
		(Test-Path -LiteralPath (Join-Path ([Environment]::ExpandEnvironmentVariables($HermesRoot)) "hermes-agent") -PathType Container)
	HermesRoot = if ([string]::IsNullOrWhiteSpace($HermesRoot)) { "" } else { [Environment]::ExpandEnvironmentVariables($HermesRoot) }
	SmokeKind = $smokeKind
	ModelPath = [string]$modelPath
	ToolName = $(if ($endpointSummary) { [string]$endpointSummary.ToolName } else { "" })
	ToolCallEncoding = $(if ($endpointSummary) { [string]$endpointSummary.ToolCallEncoding } else { "" })
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
				ToolExecutionBacked = [bool]$summary.ToolExecutionBacked
				ToolName = [string]$summary.ToolName
				ToolCallEncoding = [string]$summary.ToolCallEncoding
				HermesInstalled = [bool]$summary.HermesInstalled
				HermesRoot = [string]$summary.HermesRoot
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
