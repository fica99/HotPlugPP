param(
    [Parameter(Mandatory = $true)]
    [int]$IssueNumber,
    [string]$IssueContextFile = "",
    [string]$BaseRoot = ".",
    [string]$HandoffRoot = "..\HotPlugPP-handoff",
    [string]$Model = "",
    [switch]$PublishToGitHub,
    [switch]$DryRun,
    [int]$MaxIterations = 3
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-CommandPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,
        [string[]]$FallbackPaths = @()
    )

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    foreach ($path in $FallbackPaths) {
        if ($path -and (Test-Path $path)) {
            return $path
        }
    }

    return $null
}

function Require-Command {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,
        [string[]]$FallbackPaths = @()
    )

    $resolved = Resolve-CommandPath -Name $Name -FallbackPaths $FallbackPaths
    if (-not $resolved) {
        throw "Required command not found: $Name"
    }

    return $resolved
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

    if ($script:GhCommand) {
        return & $script:GhCommand issue view $IssueNumber --comments
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
        Set-Content -Path $OutputFile -Value "[DRY RUN] $Role was not executed.`nSTATUS: PASS`nSTATUS: READY" -Encoding UTF8
        return
    }

    $args = @("exec", "-C", $RoleDir, "--output-last-message", $OutputFile, "-")
    if ($Model) {
        $args += @("-m", $Model)
    }

    Get-Content -Raw $promptFile | & $script:CodexCommand @args
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

    if (-not $script:GhCommand) {
        Write-Warning "Skipping GitHub comment for ${Role}: gh not found."
        return
    }

    $bodyFile = Join-Path $HandoffDir ("issue-$IssueNumber-$Role-comment.md")
    $header = "[ROLE: $Role]`nIssue: #$IssueNumber`n"
    $content = Get-Content -Raw $OutputFile
    Set-Content -Path $bodyFile -Value ($header + "`n" + $content) -Encoding UTF8

    & $script:GhCommand issue comment $IssueNumber --body-file $bodyFile | Out-Null
}

function Test-AgentPass {
    param(
        [Parameter(Mandatory = $true)]
        [string]$OutputFile,
        [Parameter(Mandatory = $true)]
        [string]$PassStatus
    )
    if (-not (Test-Path $OutputFile)) { return $false }
    $content = Get-Content -Raw $OutputFile
    # Structured JSON handoff: AGENT_STATUS: {"status":"PASS",...}
    if ($content -match 'AGENT_STATUS:\s*(\{[^\}]+\})') {
        try {
            $json = $Matches[1] | ConvertFrom-Json
            return ($json.status -eq $PassStatus)
        } catch { }
    }
    # Legacy fallback: STATUS: PASS / STATUS: READY
    return [bool]($content -match "STATUS:\s*$([regex]::Escape($PassStatus))")
}

function Sync-WorktreeFromSource {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SourceDir,
        [Parameter(Mandatory = $true)]
        [string]$DestDir
    )
    $pending = & git -C $SourceDir status --porcelain 2>$null
    if ($pending) {
        & git -C $SourceDir add -A | Out-Null
        & git -C $SourceDir commit -m "auto: agent checkpoint" | Out-Null
    }
    & git -C $DestDir fetch $SourceDir HEAD 2>$null | Out-Null
    & git -C $DestDir merge FETCH_HEAD --no-edit 2>$null | Out-Null
}

$ghFallbacks = @(
    "C:\Program Files\GitHub CLI\gh.exe",
    (Join-Path $env:LOCALAPPDATA "Programs\GitHub CLI\gh.exe")
)

$script:CodexCommand = Require-Command -Name "codex"
$script:GhCommand = Resolve-CommandPath -Name "gh" -FallbackPaths $ghFallbacks

$rootPath = (Resolve-Path $BaseRoot).Path
$HandoffDir = Join-Path $rootPath $HandoffRoot
New-Item -ItemType Directory -Force -Path $HandoffDir | Out-Null

$issueContext = Get-IssueContext
$issueFile = Join-Path $HandoffDir ("issue-$IssueNumber-context.md")
Set-Content -Path $issueFile -Value $issueContext -Encoding UTF8

$plannerDir = Resolve-RolePath -Role "planner"
$implementerDir = Resolve-RolePath -Role "implementer"
$testerDir = Resolve-RolePath -Role "tester"
$securityCheckerDir = Resolve-RolePath -Role "security-checker"
$reviewerDir = Resolve-RolePath -Role "reviewer"
$docCheckerDir = Resolve-RolePath -Role "doc-checker"

$plannerOut = Join-Path $HandoffDir ("issue-$IssueNumber-planner.md")
$implementerOut = Join-Path $HandoffDir ("issue-$IssueNumber-implementer.md")
$testerOut = Join-Path $HandoffDir ("issue-$IssueNumber-tester.md")
$securityCheckerOut = Join-Path $HandoffDir ("issue-$IssueNumber-security-checker.md")
$reviewerOut = Join-Path $HandoffDir ("issue-$IssueNumber-reviewer.md")
$docCheckerOut = Join-Path $HandoffDir ("issue-$IssueNumber-doc-checker.md")

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
- End the output with exactly one JSON line:
  AGENT_STATUS: {"status":"PASS","findings":N}
  or
  AGENT_STATUS: {"status":"FAIL","findings":N}
  where N is the count of failing tests or build errors.
"@

$reviewerPrompt = @"
Issue #$IssueNumber.

Read:
- Issue context: $issueFile
- Planner handoff: $plannerOut
- Implementer handoff: $implementerOut
- Tester handoff: $testerOut
- SecurityChecker handoff: $securityCheckerOut

Task:
- Perform review with focus on bugs, regressions, API compatibility, and testing gaps.
- Return findings ordered by severity.
- End the output with exactly one JSON line:
  AGENT_STATUS: {"status":"READY","findings":N}
  or
  AGENT_STATUS: {"status":"NOT READY","findings":N}
  where N is the count of blocking findings.
"@

$securityCheckerPrompt = @"
Issue #$IssueNumber.

Read:
- Issue context: $issueFile
- Planner handoff: $plannerOut
- Implementer handoff: $implementerOut
- Tester handoff: $testerOut

Task:
- Run static analysis tools (cppcheck, clang-tidy if available) on changed source files.
- Identify security vulnerabilities, memory safety issues, and undefined behaviour.
- Document findings with severity (critical/high/medium/low) and file/line references.
- Do NOT modify source files; report findings only so the Implementer can apply fixes.
- At end, report:
  - Tools run and commands used
  - Findings by severity (critical/high/medium/low)
  - Accepted risks with justifications
- End the output with exactly one JSON line:
  AGENT_STATUS: {"status":"PASS","findings":N}
  or
  AGENT_STATUS: {"status":"FAIL","findings":N}
  where N is the count of unresolved critical or high severity findings.
"@

$implementerFixFromTesterPrompt = @"
Issue #$IssueNumber.

Read:
- Issue context: $issueFile
- Planner handoff: $plannerOut
- Previous implementer handoff: $implementerOut
- Tester handoff (contains failures): $testerOut
- SecurityChecker handoff (may not exist yet): $securityCheckerOut

Task:
- Fix all build, test, and format failures identified by the Tester.
- Fix any critical or high severity security findings identified by the SecurityChecker (if present).
- Make code changes directly in this worktree.
- At end, report:
  - Root causes identified
  - Files changed
  - Validation commands run and results
  - Remaining risks
"@

$implementerFixFromReviewerPrompt = @"
Issue #$IssueNumber.

Read:
- Issue context: $issueFile
- Planner handoff: $plannerOut
- Previous implementer handoff: $implementerOut
- Tester handoff: $testerOut
- SecurityChecker handoff: $securityCheckerOut
- Reviewer handoff (contains findings): $reviewerOut

Task:
- Address all findings raised by the Reviewer, ordered by severity.
- Make code changes directly in this worktree.
- Re-run build/test/format checks after each fix to confirm they pass.
- Skip findings already marked as resolved or not applicable.
- At end, report:
  - Findings addressed (reference reviewer finding)
  - Files changed
  - Validation commands run and results (pass/fail for each)
  - Remaining unresolved findings (if any) with justification
  - Remaining risks or assumptions
"@

$docCheckerPrompt = @"
Issue #$IssueNumber.

Read:
- Issue context: $issueFile
- Planner handoff: $plannerOut
- Implementer handoff: $implementerOut
- Tester handoff: $testerOut
- SecurityChecker handoff: $securityCheckerOut
- Reviewer handoff: $reviewerOut

Task:
- Inspect all code changes introduced for this issue (new APIs, changed behaviour, new features, removed items).
- Verify that docs/, README.md, CONTRIBUTING.md, and relevant inline comments are up-to-date.
- If documentation is missing or outdated, update it directly in this worktree.
- Generate or update CHANGELOG.md with a new entry for this issue summarising what changed.
- At end, report:
  - Documentation gaps found
  - Files updated (or "none" if already complete)
  - Validation commands run and results
  - Remaining risks or assumptions
"@

Write-Host "Running Planner..."
Invoke-CodexRole -Role "planner" -RolePrompt $plannerPrompt -RoleDir $plannerDir -OutputFile $plannerOut
Publish-HandoffComment -Role "planner" -OutputFile $plannerOut

# Implementer → Tester → SecurityChecker loop: retry Implementer if Tester or SecurityChecker fails
$implTesterIter = 0
$innerLoopPassed = $false
do {
    $implTesterIter++
    if ($implTesterIter -eq 1) {
        Write-Host "Running Implementer..."
        Invoke-CodexRole -Role "implementer" -RolePrompt $implementerPrompt -RoleDir $implementerDir -OutputFile $implementerOut
    } else {
        Write-Host "Running Implementer (retry $($implTesterIter - 1) - fixing tester/security failures)..."
        Invoke-CodexRole -Role "implementer" -RolePrompt $implementerFixFromTesterPrompt -RoleDir $implementerDir -OutputFile $implementerOut
    }
    Publish-HandoffComment -Role "implementer" -OutputFile $implementerOut
    Sync-WorktreeFromSource -SourceDir $implementerDir -DestDir $testerDir

    Write-Host "Running Tester (iteration $implTesterIter)..."
    Invoke-CodexRole -Role "tester" -RolePrompt $testerPrompt -RoleDir $testerDir -OutputFile $testerOut
    Publish-HandoffComment -Role "tester" -OutputFile $testerOut

    $testerPassed = Test-AgentPass -OutputFile $testerOut -PassStatus "PASS"
    if ($testerPassed) {
        Sync-WorktreeFromSource -SourceDir $testerDir -DestDir $securityCheckerDir

        Write-Host "Running SecurityChecker (iteration $implTesterIter)..."
        Invoke-CodexRole -Role "security-checker" -RolePrompt $securityCheckerPrompt -RoleDir $securityCheckerDir -OutputFile $securityCheckerOut
        Publish-HandoffComment -Role "security-checker" -OutputFile $securityCheckerOut

        $innerLoopPassed = Test-AgentPass -OutputFile $securityCheckerOut -PassStatus "PASS"
    }
} while (-not $innerLoopPassed -and $implTesterIter -lt $MaxIterations)

if (-not $innerLoopPassed) {
    Write-Warning "Tester/SecurityChecker did not achieve STATUS: PASS after $MaxIterations iteration(s). Continuing pipeline with last known state."
}

# Reviewer loop: if Reviewer rejects, send back to Implementer → Tester → SecurityChecker → Reviewer
$reviewIter = 0
do {
    $reviewIter++
    if ($reviewIter -gt 1) {
        Write-Host "Running Implementer (retry $($reviewIter - 1) - addressing reviewer findings)..."
        Invoke-CodexRole -Role "implementer" -RolePrompt $implementerFixFromReviewerPrompt -RoleDir $implementerDir -OutputFile $implementerOut
        Publish-HandoffComment -Role "implementer" -OutputFile $implementerOut
        Sync-WorktreeFromSource -SourceDir $implementerDir -DestDir $testerDir

        Write-Host "Running Tester (re-run after implementer retry $($reviewIter - 1))..."
        Invoke-CodexRole -Role "tester" -RolePrompt $testerPrompt -RoleDir $testerDir -OutputFile $testerOut
        Publish-HandoffComment -Role "tester" -OutputFile $testerOut

        if (-not (Test-AgentPass -OutputFile $testerOut -PassStatus "PASS")) {
            Write-Warning "Tester did not pass after implementer retry $($reviewIter - 1). Breaking reviewer loop."
            break
        }
        Sync-WorktreeFromSource -SourceDir $testerDir -DestDir $securityCheckerDir

        Write-Host "Running SecurityChecker (re-run after implementer retry $($reviewIter - 1))..."
        Invoke-CodexRole -Role "security-checker" -RolePrompt $securityCheckerPrompt -RoleDir $securityCheckerDir -OutputFile $securityCheckerOut
        Publish-HandoffComment -Role "security-checker" -OutputFile $securityCheckerOut

        if (-not (Test-AgentPass -OutputFile $securityCheckerOut -PassStatus "PASS")) {
            Write-Warning "SecurityChecker did not pass after implementer retry $($reviewIter - 1). Breaking reviewer loop."
            break
        }
    }

    Sync-WorktreeFromSource -SourceDir $securityCheckerDir -DestDir $reviewerDir
    Write-Host "Running Reviewer (iteration $reviewIter)..."
    Invoke-CodexRole -Role "reviewer" -RolePrompt $reviewerPrompt -RoleDir $reviewerDir -OutputFile $reviewerOut
    Publish-HandoffComment -Role "reviewer" -OutputFile $reviewerOut
} while (-not (Test-AgentPass -OutputFile $reviewerOut -PassStatus "READY") -and $reviewIter -lt $MaxIterations)

if (-not (Test-AgentPass -OutputFile $reviewerOut -PassStatus "READY")) {
    Write-Warning "Reviewer did not achieve STATUS: READY after $MaxIterations iteration(s). Continuing to DocChecker with last known state."
}

Write-Host "Running DocChecker..."
Invoke-CodexRole -Role "doc-checker" -RolePrompt $docCheckerPrompt -RoleDir $docCheckerDir -OutputFile $docCheckerOut
Publish-HandoffComment -Role "doc-checker" -OutputFile $docCheckerOut

if ($PublishToGitHub -and $script:GhCommand) {
    Write-Host "Creating pull request from implementer branch..."
    $implBranch = & git -C $implementerDir rev-parse --abbrev-ref HEAD 2>$null
    if ($implBranch -and $implBranch -ne "HEAD") {
        $pending = & git -C $implementerDir status --porcelain 2>$null
        if ($pending) {
            & git -C $implementerDir add -A | Out-Null
            & git -C $implementerDir commit -m "feat: automated fix for issue #$IssueNumber" | Out-Null
        }
        & git -C $implementerDir push origin $implBranch 2>&1 | Out-Null
        $prResult = & $script:GhCommand pr create `
            --title "fix: automated resolution of issue #$IssueNumber" `
            --body "Automated multi-agent pipeline fix for issue #$IssueNumber." `
            --base main `
            --head $implBranch 2>&1
        Write-Host "PR: $prResult"
        & $script:GhCommand pr merge --auto --squash 2>&1 | Out-Null
        Write-Host "Auto-merge enabled."
    }
}

Write-Host ""
Write-Host "Completed multi-agent run for issue #$IssueNumber"
Write-Host "Handoff directory: $HandoffDir"
Write-Host "Planner:          $plannerOut"
Write-Host "Implementer:      $implementerOut"
Write-Host "Tester:           $testerOut"
Write-Host "SecurityChecker:  $securityCheckerOut"
Write-Host "Reviewer:         $reviewerOut"
Write-Host "DocChecker:       $docCheckerOut"
