param(
    [Parameter(Mandatory = $true)]
    [int]$IssueNumber,
    [string]$IssueContextFile = "",
    [string]$BaseRoot = ".",
    [string]$HandoffRoot = "..\HotPlugPP-handoff",
    [string]$Model = "",
    [string]$Provider = "codex",
    [string]$SingleRole = "",
    [switch]$PublishToGitHub,
    [switch]$DryRun,
    [int]$MaxIterations = 3
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# Keep external command I/O in UTF-8 so gh JSON/text is decoded correctly on Windows.
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[Console]::InputEncoding = $utf8NoBom
[Console]::OutputEncoding = $utf8NoBom
$OutputEncoding = $utf8NoBom

function Invoke-ProcessCapture {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [string[]]$ArgumentList = @()
    )

    $quotedArgs = foreach ($arg in $ArgumentList) {
        if ($null -eq $arg) {
            '""'
            continue
        }

        $text = [string]$arg
        if ($text -eq "") {
            '""'
            continue
        }

        if ($text -notmatch '[\s"]') {
            $text
            continue
        }

        $escaped = $text -replace '(\\*)"', '$1$1\"'
        $escaped = $escaped -replace '(\\+)$', '$1$1'
        '"' + $escaped + '"'
    }

    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $FilePath
    $startInfo.Arguments = [string]::Join(' ', $quotedArgs)
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    [void]$process.Start()

    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()

    $process.WaitForExit()
    $stdoutTask.Wait()
    $stderrTask.Wait()

    $stdout = [string]$stdoutTask.Result
    $stderr = [string]$stderrTask.Result

    return [pscustomobject]@{
        ExitCode = $process.ExitCode
        StdOut   = $stdout.TrimEnd("`r", "`n")
        StdErr   = $stderr.TrimEnd("`r", "`n")
    }
}

function Invoke-GitCapture {
    param([string[]]$ArgumentList = @())
    return Invoke-ProcessCapture -FilePath "git" -ArgumentList $ArgumentList
}

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
        [string]$Role,
        [switch]$Lenient
    )

    $leaf = Split-Path -Leaf (Resolve-Path $BaseRoot)
    $issueSuffix = if ($IssueNumber -gt 0) { "-issue-$IssueNumber" } else { "" }
    $candidate = Join-Path (Split-Path -Parent (Resolve-Path $BaseRoot)) ($leaf + "-" + $Role + $issueSuffix)
    if (-not (Test-Path $candidate)) {
        if ($Lenient) { return $candidate }
        throw "Role worktree not found: $candidate. Run scripts/setup-agent-worktrees.ps1 -IssueNumber $IssueNumber first."
    }
    return (Resolve-Path $candidate).Path
}

function Get-IssueContext {
    if ($IssueContextFile -and (Test-Path $IssueContextFile)) {
        return Get-Content -Raw $IssueContextFile
    }

    if ($script:GhCommand) {
        $issueJson = & $script:GhCommand issue view $IssueNumber --json title,body,url
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to read issue #$IssueNumber from GitHub."
        }

        $issue = $issueJson | ConvertFrom-Json
        $body = if ($issue.body) { $issue.body.Trim() } else { "(no body provided)" }

        return @"
Issue #$IssueNumber
Title: $($issue.title)
URL: $($issue.url)

Body:
$body
"@
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

    if ($Provider -eq "github-copilot") {
        $promptContent = Get-Content -Raw $promptFile
        # Default model for GitHub Copilot provider; use -Model to override (codex provider uses its own default)
        $copilotModel = if ($Model) { $Model } else { "gpt-4o" }
        $requestBody = @{
            messages = @(@{ role = "user"; content = $promptContent })
            model    = $copilotModel
        } | ConvertTo-Json -Depth 5 -Compress

        $tmpErr = [System.IO.Path]::GetTempFileName()
        $responseJson = $requestBody | & $script:GhCommand api /copilot/chat/completions `
            --method POST `
            --input - `
            --header "Content-Type: application/json" 2>$tmpErr
        $errContent = Get-Content -Raw $tmpErr -ErrorAction SilentlyContinue
        Remove-Item $tmpErr -ErrorAction SilentlyContinue

        if ($LASTEXITCODE -ne 0) {
            throw "GitHub Copilot API call failed for role '$Role': $errContent"
        }

        $parsed = $responseJson | ConvertFrom-Json
        if (-not $parsed.choices -or $parsed.choices.Count -eq 0 -or -not $parsed.choices[0].message) {
            throw "Unexpected GitHub Copilot API response structure for role '$Role': $responseJson"
        }
        $responseContent = $parsed.choices[0].message.content
        Set-Content -Path $OutputFile -Value $responseContent -Encoding UTF8
    } else {
        $codeArgs = @("exec", "-C", $RoleDir, "--output-last-message", $OutputFile, "-")
        if ($Model) {
            $codeArgs += @("-m", $Model)
        }

        Get-Content -Raw $promptFile | & $script:CodexCommand @codeArgs
        if ($LASTEXITCODE -ne 0) {
            throw "Codex execution failed for role '$Role'."
        }
    }

    if (-not (Test-Path $OutputFile)) {
        throw "Role '$Role' did not produce the expected handoff file: $OutputFile"
    }
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
    # Structured JSON handoff: AGENT_STATUS: {"status":"PASS",...} — match the whole line to avoid nested-brace issues
    $statusLine = $content -split '\r?\n' | Where-Object { $_ -match '^\s*AGENT_STATUS:\s*(\{.+\})\s*$' } | Select-Object -Last 1
    if ($statusLine -match '^\s*AGENT_STATUS:\s*(\{.+\})\s*$') {
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
    $statusResult = Invoke-GitCapture -ArgumentList @("-C", $SourceDir, "status", "--porcelain")
    if ($statusResult.ExitCode -ne 0) {
        throw "Failed to inspect git status in '$SourceDir': $($statusResult.StdErr)"
    }

    $pending = $statusResult.StdOut
    if ($pending) {
        $addResult = Invoke-GitCapture -ArgumentList @("-C", $SourceDir, "add", "-A")
        if ($addResult.ExitCode -ne 0) {
            throw "Failed to stage changes in '$SourceDir': $($addResult.StdErr)"
        }

        $commitResult = Invoke-GitCapture -ArgumentList @("-C", $SourceDir, "commit", "-m", "auto: agent checkpoint")
        if ($commitResult.ExitCode -ne 0) {
            throw "Failed to create auto-checkpoint commit in '$SourceDir': $($commitResult.StdErr)"
        }
    }

    $fetchResult = Invoke-GitCapture -ArgumentList @("-C", $DestDir, "fetch", "--quiet", $SourceDir, "HEAD")
    if ($fetchResult.ExitCode -ne 0) {
        throw "Worktree sync fetch from '$SourceDir' to '$DestDir' failed. $($fetchResult.StdErr)"
    }

    $mergeResult = Invoke-GitCapture -ArgumentList @("-C", $DestDir, "merge", "FETCH_HEAD", "--no-edit")
    if ($mergeResult.ExitCode -ne 0) {
        throw "Worktree sync from '$SourceDir' to '$DestDir' encountered issues: $($mergeResult.StdErr)"
    }
}

function Ensure-GitSafeDirectory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (-not (Test-Path $Path)) {
        return
    }

    $resolvedPath = (Resolve-Path $Path).Path
    $accessCheck = Invoke-GitCapture -ArgumentList @("-C", $resolvedPath, "status", "--porcelain")
    if ($accessCheck.ExitCode -eq 0) {
        return
    }

    if ($accessCheck.StdErr -notmatch "dubious ownership") {
        throw "Failed to access git worktree '$resolvedPath': $($accessCheck.StdErr)"
    }

    $existingResult = Invoke-GitCapture -ArgumentList @("config", "--global", "--get-all", "safe.directory")
    if ($existingResult.ExitCode -gt 1) {
        throw "Failed to read git safe.directory entries: $($existingResult.StdErr)"
    }

    $existing = if ($existingResult.StdOut) { $existingResult.StdOut -split '\r?\n' } else { @() }
    if ($existing -contains $resolvedPath) {
        return
    }

    $addSafeResult = Invoke-GitCapture -ArgumentList @("config", "--global", "--add", "safe.directory", $resolvedPath)
    if ($addSafeResult.ExitCode -ne 0) {
        throw "Failed to add git safe.directory for '$resolvedPath': $($addSafeResult.StdErr)"
    }
}

$ghFallbacks = @(
    "C:\Program Files\GitHub CLI\gh.exe"
)
if ($env:LOCALAPPDATA) {
    $ghFallbacks += Join-Path $env:LOCALAPPDATA "Programs\GitHub CLI\gh.exe"
}

$script:CodexCommand = if ($Provider -ne "github-copilot") { Require-Command -Name "codex" } else { $null }
$script:GhCommand = Resolve-CommandPath -Name "gh" -FallbackPaths $ghFallbacks
if ($Provider -eq "github-copilot" -and -not $script:GhCommand) {
    throw "Required command not found: gh (needed for github-copilot provider)"
}

$rootPath = (Resolve-Path $BaseRoot).Path
$HandoffDir = Join-Path $rootPath $HandoffRoot
New-Item -ItemType Directory -Force -Path $HandoffDir | Out-Null
$HandoffDir = (Resolve-Path $HandoffDir).Path

$issueContext = Get-IssueContext
$issueFile = Join-Path $HandoffDir ("issue-$IssueNumber-context.md")
Set-Content -Path $issueFile -Value $issueContext -Encoding UTF8

$allowMissingWorktrees = [bool]$SingleRole
$plannerDir = Resolve-RolePath -Role "planner" -Lenient:$allowMissingWorktrees
$implementerDir = Resolve-RolePath -Role "implementer" -Lenient:$allowMissingWorktrees
$testerDir = Resolve-RolePath -Role "tester" -Lenient:$allowMissingWorktrees
$securityCheckerDir = Resolve-RolePath -Role "security-checker" -Lenient:$allowMissingWorktrees
$reviewerDir = Resolve-RolePath -Role "reviewer" -Lenient:$allowMissingWorktrees
$docCheckerDir = Resolve-RolePath -Role "doc-checker" -Lenient:$allowMissingWorktrees

@(
    $plannerDir,
    $implementerDir,
    $testerDir,
    $securityCheckerDir,
    $reviewerDir,
    $docCheckerDir
) | ForEach-Object {
    Ensure-GitSafeDirectory -Path $_
}

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

function Publish-PullRequest {
    if (-not ($PublishToGitHub -and $script:GhCommand)) { return }
    Write-Host "Creating pull request from implementer branch..."
    $branchResult = Invoke-GitCapture -ArgumentList @("-C", $implementerDir, "rev-parse", "--abbrev-ref", "HEAD")
    if ($branchResult.ExitCode -ne 0) {
        throw "Failed to determine implementer branch: $($branchResult.StdErr)"
    }

    $implBranch = $branchResult.StdOut
    if ($implBranch -and $implBranch -ne "HEAD") {
        $statusResult = Invoke-GitCapture -ArgumentList @("-C", $implementerDir, "status", "--porcelain")
        if ($statusResult.ExitCode -ne 0) {
            throw "Failed to inspect implementer worktree status: $($statusResult.StdErr)"
        }

        $pending = $statusResult.StdOut
        if ($pending) {
            $addResult = Invoke-GitCapture -ArgumentList @("-C", $implementerDir, "add", "-A")
            if ($addResult.ExitCode -ne 0) {
                throw "Failed to stage implementer changes: $($addResult.StdErr)"
            }

            $commitResult = Invoke-GitCapture -ArgumentList @("-C", $implementerDir, "commit", "-m", "feat: automated fix for issue #$IssueNumber")
            if ($commitResult.ExitCode -ne 0) {
                throw "Failed to create implementer publish commit in '$implementerDir': $($commitResult.StdErr)"
            }
        }

        $pushResult = Invoke-GitCapture -ArgumentList @("-C", $implementerDir, "push", "origin", $implBranch)
        if ($pushResult.ExitCode -ne 0) {
            Write-Warning "Failed to push branch '$implBranch'. Skipping PR creation. Error: $($pushResult.StdErr)"
        } else {
            $prResult = & $script:GhCommand pr create `
                --title "fix: automated resolution of issue #$IssueNumber" `
                --body "Closes #$IssueNumber" `
                --base main `
                --head $implBranch 2>&1
            if ($LASTEXITCODE -eq 0) {
                Write-Host "PR created: $prResult"
                $mergeOutput = & $script:GhCommand pr merge --auto --squash 2>&1
                if ($LASTEXITCODE -ne 0) {
                    throw "PR was created but auto-merge could not be enabled: $mergeOutput"
                }
                Write-Host "Auto-merge enabled."
            } else {
                Write-Warning "PR creation failed (PR may already exist). Output: $prResult"
            }
        }
    }
}

function Publish-RoleBranch {
    param([Parameter(Mandatory = $true)][string]$RoleDir)
    if (-not $PublishToGitHub) { return }
    $branchResult = Invoke-GitCapture -ArgumentList @("-C", $RoleDir, "rev-parse", "--abbrev-ref", "HEAD")
    if ($branchResult.ExitCode -ne 0) {
        throw "Failed to determine role branch in '$RoleDir': $($branchResult.StdErr)"
    }

    $branch = $branchResult.StdOut
    if (-not $branch -or $branch -eq "HEAD") { return }
    $statusResult = Invoke-GitCapture -ArgumentList @("-C", $RoleDir, "status", "--porcelain")
    if ($statusResult.ExitCode -ne 0) {
        throw "Failed to inspect role worktree status in '$RoleDir': $($statusResult.StdErr)"
    }

    $pending = $statusResult.StdOut
    if ($pending) {
        $addResult = Invoke-GitCapture -ArgumentList @("-C", $RoleDir, "add", "-A")
        if ($addResult.ExitCode -ne 0) {
            throw "Failed to stage role changes in '$RoleDir': $($addResult.StdErr)"
        }

        $commitResult = Invoke-GitCapture -ArgumentList @("-C", $RoleDir, "commit", "-m", "auto: agent checkpoint")
        if ($commitResult.ExitCode -ne 0) {
            throw "Failed to create auto-checkpoint commit in '$RoleDir': $($commitResult.StdErr)"
        }
    }

    $pushResult = Invoke-GitCapture -ArgumentList @("-C", $RoleDir, "push", "origin", $branch)
    if ($pushResult.ExitCode -ne 0) {
        throw "Failed to push role branch '$branch' from '$RoleDir': $($pushResult.StdErr)"
    }
}

if ($SingleRole) {
    switch ($SingleRole.ToLower()) {
        "planner" {
            Write-Host "Running Planner (single-role mode)..."
            Invoke-CodexRole -Role "planner" -RolePrompt $plannerPrompt -RoleDir $plannerDir -OutputFile $plannerOut
            Publish-HandoffComment -Role "planner" -OutputFile $plannerOut
            Publish-RoleBranch -RoleDir $plannerDir
        }
        "implementer" {
            Write-Host "Running Implementer (single-role mode)..."
            Invoke-CodexRole -Role "implementer" -RolePrompt $implementerPrompt -RoleDir $implementerDir -OutputFile $implementerOut
            Publish-HandoffComment -Role "implementer" -OutputFile $implementerOut
            Publish-RoleBranch -RoleDir $implementerDir
        }
        "tester" {
            if (Test-Path $implementerDir) { Sync-WorktreeFromSource -SourceDir $implementerDir -DestDir $testerDir }
            Write-Host "Running Tester (single-role mode)..."
            Invoke-CodexRole -Role "tester" -RolePrompt $testerPrompt -RoleDir $testerDir -OutputFile $testerOut
            Publish-HandoffComment -Role "tester" -OutputFile $testerOut
            Publish-RoleBranch -RoleDir $testerDir
        }
        "security-checker" {
            if (Test-Path $testerDir) { Sync-WorktreeFromSource -SourceDir $testerDir -DestDir $securityCheckerDir }
            Write-Host "Running SecurityChecker (single-role mode)..."
            Invoke-CodexRole -Role "security-checker" -RolePrompt $securityCheckerPrompt -RoleDir $securityCheckerDir -OutputFile $securityCheckerOut
            Publish-HandoffComment -Role "security-checker" -OutputFile $securityCheckerOut
            Publish-RoleBranch -RoleDir $securityCheckerDir
        }
        "reviewer" {
            if (Test-Path $securityCheckerDir) { Sync-WorktreeFromSource -SourceDir $securityCheckerDir -DestDir $reviewerDir }
            Write-Host "Running Reviewer (single-role mode)..."
            Invoke-CodexRole -Role "reviewer" -RolePrompt $reviewerPrompt -RoleDir $reviewerDir -OutputFile $reviewerOut
            Publish-HandoffComment -Role "reviewer" -OutputFile $reviewerOut
            Publish-RoleBranch -RoleDir $reviewerDir
        }
        "doc-checker" {
            if (Test-Path $reviewerDir) { Sync-WorktreeFromSource -SourceDir $reviewerDir -DestDir $docCheckerDir }
            Write-Host "Running DocChecker (single-role mode)..."
            Invoke-CodexRole -Role "doc-checker" -RolePrompt $docCheckerPrompt -RoleDir $docCheckerDir -OutputFile $docCheckerOut
            Publish-HandoffComment -Role "doc-checker" -OutputFile $docCheckerOut
            Sync-WorktreeFromSource -SourceDir $docCheckerDir -DestDir $implementerDir
            Publish-PullRequest
        }
        default {
            throw "Unknown role: '$SingleRole'. Valid roles: planner, implementer, tester, security-checker, reviewer, doc-checker"
        }
    }
} else {

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
Sync-WorktreeFromSource -SourceDir $reviewerDir -DestDir $docCheckerDir
Invoke-CodexRole -Role "doc-checker" -RolePrompt $docCheckerPrompt -RoleDir $docCheckerDir -OutputFile $docCheckerOut
Publish-HandoffComment -Role "doc-checker" -OutputFile $docCheckerOut
Sync-WorktreeFromSource -SourceDir $docCheckerDir -DestDir $implementerDir

Publish-PullRequest

} # end else (full pipeline)

Write-Host ""
Write-Host "Completed multi-agent run for issue #$IssueNumber"
Write-Host "Handoff directory: $HandoffDir"
Write-Host "Planner:          $plannerOut"
Write-Host "Implementer:      $implementerOut"
Write-Host "Tester:           $testerOut"
Write-Host "SecurityChecker:  $securityCheckerOut"
Write-Host "Reviewer:         $reviewerOut"
Write-Host "DocChecker:       $docCheckerOut"
