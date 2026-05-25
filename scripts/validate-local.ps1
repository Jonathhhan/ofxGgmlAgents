param()

$ErrorActionPreference = "Stop"

function Write-Step {
	param([string]$Message)
	Write-Host "==> $Message"
}

function Assert-Path {
	param(
		[string]$Path,
		[string]$Label,
		[switch]$Directory
	)

	if ($Directory) {
		if (!(Test-Path -LiteralPath $Path -PathType Container)) {
			throw "$Label was not found: $Path"
		}
	} elseif (!(Test-Path -LiteralPath $Path -PathType Leaf)) {
		throw "$Label was not found: $Path"
	}
}

function Assert-FileContains {
	param(
		[string]$Path,
		[string]$Pattern,
		[string]$Label
	)

	$content = Get-Content -LiteralPath $Path -Raw
	if ($content -notmatch $Pattern) {
		throw "$Label did not contain expected pattern: $Pattern"
	}
}
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$addonRoot = Split-Path -Parent $scriptRoot
$addonsRoot = Split-Path -Parent $addonRoot

Write-Step "Checking addon skeleton"
Assert-Path (Join-Path $addonRoot "addon_config.mk") "addon config"
Assert-Path (Join-Path $addonRoot "README.md") "README"
Assert-Path (Join-Path $addonRoot "LICENSE") "license"
Assert-Path (Join-Path $addonRoot "docs\AGENT_WORKFLOWS.md") "agent workflow docs"
Assert-FileContains (Join-Path $addonRoot "README.md") "AGENT_WORKFLOWS.md" "README"
Assert-FileContains (Join-Path $addonRoot "docs\AGENT_WORKFLOWS.md") "Planning handoff" "agent workflow docs"
Assert-FileContains (Join-Path $addonRoot "docs\AGENT_WORKFLOWS.md") "tool-call boundaries" "agent workflow docs"
Assert-FileContains (Join-Path $addonRoot "docs\AGENT_WORKFLOWS.md") "runtime behavior out of scope" "agent workflow docs"
Assert-Path (Join-Path $addonRoot "src\ofxGgmlAgents.h") "public header"
Assert-Path (Join-Path $addonRoot "src\ofxGgmlAgentsVersion.h") "version header"
Assert-FileContains (Join-Path $addonRoot "src\ofxGgmlAgents.h") "ofxGgmlAgentsVersion.h" "public header"
Assert-FileContains (Join-Path $addonRoot "src\ofxGgmlAgentsVersion.h") "OFXGGML_AGENTS_VERSION_STRING" "version header"
Assert-Path (Join-Path $addonRoot "src\ofxGgmlAgents\ofxGgmlAgentsTypes.h") "types header"
Assert-Path (Join-Path $addonRoot "src\ofxGgmlAgents\ofxGgmlAgentsUtils.h") "utility header"
Assert-Path (Join-Path $addonRoot "src\ofxGgmlAgents\ofxGgmlAgentsUtils.cpp") "utility source"

Write-Step "Checking dependency layout"
Assert-Path (Join-Path $addonsRoot "ofxGgmlCore") "sibling ofxGgmlCore addon" -Directory
Assert-Path (Join-Path $addonsRoot "ofxImGui") "sibling ofxImGui addon for examples" -Directory

Write-Step "Checking example layout"
$exampleRoot = Join-Path $addonRoot "ofxGgmlAgentsPlannerExample"
Assert-Path $exampleRoot "root-level smoke example" -Directory
Assert-Path (Join-Path $exampleRoot "addons.make") "smoke example addons.make"
Assert-FileContains (Join-Path $exampleRoot "addons.make") "(?m)^ofxImGui\s*$" "smoke example addons.make"
Assert-Path (Join-Path $exampleRoot "src\main.cpp") "smoke example main.cpp"
Assert-Path (Join-Path $exampleRoot "src\ofApp.h") "smoke example ofApp.h"
Assert-Path (Join-Path $exampleRoot "src\ofApp.cpp") "smoke example ofApp.cpp"
Assert-Path (Join-Path $addonRoot "tests\CMakeLists.txt") "test CMakeLists"
Assert-Path (Join-Path $addonRoot "tests\test_main.cpp") "test source"
Assert-Path (Join-Path $scriptRoot "doctor-agents.ps1") "Agents doctor script"
Assert-Path (Join-Path $scriptRoot "doctor-agents.bat") "Agents doctor Windows wrapper"
Assert-Path (Join-Path $scriptRoot "doctor-agents.sh") "Agents doctor shell wrapper"
Assert-Path (Join-Path $scriptRoot "test-doctor-agents.ps1") "Agents doctor smoke test"
Assert-Path (Join-Path $scriptRoot "run-agents-runtime-smoke.ps1") "Agents runtime smoke script"
Assert-Path (Join-Path $scriptRoot "run-agents-runtime-smoke.bat") "Agents runtime smoke Windows wrapper"
Assert-Path (Join-Path $scriptRoot "run-agents-runtime-smoke.sh") "Agents runtime smoke shell wrapper"
Assert-Path (Join-Path $scriptRoot "test-agents-runtime-smoke.ps1") "Agents runtime smoke contract test"

$nestedExamples = Join-Path $addonRoot "examples"
if (Test-Path -LiteralPath $nestedExamples -PathType Container) {
	throw "Examples should live at the addon root, not under: $nestedExamples"
}

Write-Step "Checking generated artifact hygiene"
$forbidden = @(
	"build",
	".vs",
	"ofxGgmlAgentsPlannerExample\bin",
	"ofxGgmlAgentsPlannerExample\obj",
	"ofxGgmlAgentsPlannerExample\.vs",
	"models"
)

foreach ($relative in $forbidden) {
	$tracked = git -C $addonRoot ls-files -- $relative
	if ($tracked) {
		throw "Generated or local-only path is tracked and should not be committed here: $relative"
	}
}

Write-Step "Checking Agents doctor"
& (Join-Path $scriptRoot "test-doctor-agents.ps1")
if (!$?) {
	throw "Agents doctor smoke test failed"
}

Write-Step "Checking Agents runtime smoke contract"
& (Join-Path $scriptRoot "test-agents-runtime-smoke.ps1")

Write-Step "Running headless tests"
& (Join-Path $scriptRoot "test-addon.ps1")
if ($LASTEXITCODE -ne 0) {
	throw "Headless tests failed with exit code $LASTEXITCODE"
}


Write-Step "Checking workflow callers"
$callerWorkflows = @("release-check.yml", "backend-runtime-check.yml", "release-gate.yml")
$workflowDir = Join-Path $addonRoot ".github\workflows"
foreach ($wf in $callerWorkflows) {
    $wfPath = Join-Path $workflowDir $wf
    if (!(Test-Path -LiteralPath $wfPath -PathType Leaf)) {
        throw "Workflow caller not found: $wfPath"
    }
    $content = Get-Content -LiteralPath $wfPath -Raw
    if ($content -notmatch 'uses:.*ofxGgmlWorkflows') {
        throw "Workflow caller $wf does not reference ofxGgmlWorkflows reusable workflow"
    }
}
Write-Step 'ofxGgmlAgents local validation passed'
