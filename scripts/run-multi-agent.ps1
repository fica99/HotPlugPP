param(
    [Parameter(Mandatory = $true)]
    [int]$IssueNumber,
    [string]$IssueContextFile = "",
    [string]$BaseRoot = ".",
    [string]$HandoffRoot = "..\HotPlugPP-handoff",
    [string]$Model = "",
    [switch]$PublishToGitHub,
    [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Require-Command {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command not found: $Name"
    }
}

function Resolve-RolePath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Role
    )

    $leaf = Split-Path -Leaf (Resolve-Path $BaseRoot)
    $candidate = Join-Path (Split-Path -Parent (Resolve-Path $BaseRoot)) ($leaf + "-" + $Role)
    if (-not (Test-Path $candidate)) {
        throw "Role worktree not found: $candidate. Run scripts/setup-agent-worktrees.ps1 first."
    }
    return (Resolve-Path $candidate).Path
}

function Get-IssueContext {
    if ($IssueContextFile -and (Test-Path $IssueContextFile)) {
        return Get-Content -Raw $IssueContextFile
    }

    $hasGh = [bool](Get-Command gh -ErrorAction SilentlyContinue)
    if ($hasGh) {
        return gh issue view $IssueNumber --comments
    }

    $template = @"
Issue #$IssueNumber context is missing.

Either:
1) install GitHub CLI and run 'gh auth login', or
2) provide -IssueContextFile with issue title/body/acceptance criteria.
"@
    throw $template
}

function Invoke-CodexRole {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Role,
        [Parameter(Mandatory = $true)]
        [string]$RolePrompt,
        [Parameter(Mandatory = $true)]
        [string]$RoleDir,
        [Parameter(Mandatory = $true)]
        [string]$OutputFile
    )

    $promptFile = Join-Path $HandoffDir ("issue-$IssueNumber-$Role-prompt.md")
    Set-Content -Path $promptFile -Value $RolePrompt -Encoding UTF8

    if ($DryRun) {
        Set-Content -Path $OutputFile -Value "[DRY RUN] $Role was not executed." -Encoding UTF8
        return
    }

    $args = @("exec", "-C", $RoleDir, "--output-last-message", $OutputFile, "-")
    if ($Model) {
        $args += @("-m", $Model)
    }

    Get-Content -Raw $promptFile | & codex @args
}

function Publish-HandoffComment {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Role,
        [Parameter(Mandatory = $true)]
        [string]$OutputFile
    )

    if (-not $PublishToGitHub) {
        return
    }

    if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
        Write-Warning "Skipping GitHub comment for ${Role}: gh not found."
        return
    }

    $bodyFile = Join-Path $HandoffDir ("issue-$IssueNumber-$Role-comment.md")
    $header = "[ROLE: $Role]`nIssue: #$IssueNumber`n"
    $content = Get-Content -Raw $OutputFile
    Set-Content -Path $bodyFile -Value ($header + "`n" + $content) -Encoding UTF8

    gh issue comment $IssueNumber --body-file $bodyFile | Out-Null
}

Require-Command -Name "codex"

$rootPath = (Resolve-Path $BaseRoot).Path
$HandoffDir = Join-Path $rootPath $HandoffRoot
New-Item -ItemType Directory -Force -Path $HandoffDir | Out-Null

$issueContext = Get-IssueContext
$issueFile = Join-Path $HandoffDir ("issue-$IssueNumber-context.md")
Set-Content -Path $issueFile -Value $issueContext -Encoding UTF8

$plannerDir = Resolve-RolePath -Role "planner"
$implementerDir = Resolve-RolePath -Role "implementer"
$testerDir = Resolve-RolePath -Role "tester"
$reviewerDir = Resolve-RolePath -Role "reviewer"

$plannerOut = Join-Path $HandoffDir ("issue-$IssueNumber-planner.md")
$implementerOut = Join-Path $HandoffDir ("issue-$IssueNumber-implementer.md")
$testerOut = Join-Path $HandoffDir ("issue-$IssueNumber-tester.md")
$reviewerOut = Join-Path $HandoffDir ("issue-$IssueNumber-reviewer.md")

$plannerPrompt = @"
Issue #$IssueNumber.

Read issue context from:
$issueFile

Task:
- Produce implementation plan with scope, non-goals, file-level change list, risks, acceptance checks, and DoD commands.
- Keep it concise and actionable.
- End with:
  - Summary of changes: none (planning stage)
  - Files changed: none
  - Validation commands run: none
  - Remaining assumptions/risks
"@

$implementerPrompt = @"
Issue #$IssueNumber.

Read:
- Issue context: $issueFile
- Planner handoff: $plannerOut

Task:
- Implement according to planner handoff.
- Make code changes directly in this worktree.
- Preserve existing ABI unless issue explicitly requires changes.
- At end, report:
  - Files changed
  - Validation commands run and results
  - Remaining risks
"@

$testerPrompt = @"
Issue #$IssueNumber.

Read:
- Issue context: $issueFile
- Planner handoff: $plannerOut
- Implementer handoff: $implementerOut

Task:
- Add/update tests for acceptance criteria.
- Run build/test/format checks relevant to the change.
- If failures occur, fix test or code issues in this worktree.
- At end, report:
  - Files changed
  - Command results (pass/fail)
  - Residual risks
"@

$reviewerPrompt = @"
Issue #$IssueNumber.

Read:
- Issue context: $issueFile
- Planner handoff: $plannerOut
- Implementer handoff: $implementerOut
- Tester handoff: $testerOut

Task:
- Perform review with focus on bugs, regressions, API compatibility, and testing gaps.
- Return findings ordered by severity.
- End with final verdict: Ready / Not ready.
"@

Write-Host "Running Planner..."
Invoke-CodexRole -Role "planner" -RolePrompt $plannerPrompt -RoleDir $plannerDir -OutputFile $plannerOut
Publish-HandoffComment -Role "planner" -OutputFile $plannerOut

Write-Host "Running Implementer..."
Invoke-CodexRole -Role "implementer" -RolePrompt $implementerPrompt -RoleDir $implementerDir -OutputFile $implementerOut
Publish-HandoffComment -Role "implementer" -OutputFile $implementerOut

Write-Host "Running Tester..."
Invoke-CodexRole -Role "tester" -RolePrompt $testerPrompt -RoleDir $testerDir -OutputFile $testerOut
Publish-HandoffComment -Role "tester" -OutputFile $testerOut

Write-Host "Running Reviewer..."
Invoke-CodexRole -Role "reviewer" -RolePrompt $reviewerPrompt -RoleDir $reviewerDir -OutputFile $reviewerOut
Publish-HandoffComment -Role "reviewer" -OutputFile $reviewerOut

Write-Host ""
Write-Host "Completed multi-agent run for issue #$IssueNumber"
Write-Host "Handoff directory: $HandoffDir"
Write-Host "Planner:     $plannerOut"
Write-Host "Implementer: $implementerOut"
Write-Host "Tester:      $testerOut"
Write-Host "Reviewer:    $reviewerOut"
