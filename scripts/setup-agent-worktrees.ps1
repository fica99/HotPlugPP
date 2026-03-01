param(
    [string]$BaseBranch = "main",
    [string]$Prefix = "agent",
    [string]$WorktreeRoot = "..",
    [int]$IssueNumber = 0
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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

$pruneResult = Invoke-GitCapture -ArgumentList @("worktree", "prune")
if ($pruneResult.ExitCode -ne 0) {
    throw "Failed to prune stale worktrees: $($pruneResult.StdErr)"
}

function New-AgentWorktree {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Role
    )

    $issueSuffix = if ($IssueNumber -gt 0) { "-issue-$IssueNumber" } else { "" }
    $branchName = if ($IssueNumber -gt 0) { "$Prefix/issue-$IssueNumber/$Role" } else { "$Prefix/$Role/$BaseBranch" }
    $worktreePath = Join-Path $WorktreeRoot ("HotPlugPP-" + $Role + $issueSuffix)

    if (Test-Path $worktreePath) {
        Write-Host "Removing existing worktree at '$worktreePath'..."
        $removeResult = Invoke-GitCapture -ArgumentList @("worktree", "remove", "--force", $worktreePath)
        if ($removeResult.ExitCode -ne 0) {
            Write-Warning "Could not remove existing worktree at '$worktreePath': $($removeResult.StdErr)"
        }
    }

    $branchListResult = Invoke-GitCapture -ArgumentList @("branch", "--list", $branchName)
    if ($branchListResult.ExitCode -ne 0) {
        throw "Failed to inspect local branches: $($branchListResult.StdErr)"
    }

    $localBranchExists = [bool]$branchListResult.StdOut
    if (-not $localBranchExists) {
        $remoteCheckResult = Invoke-GitCapture -ArgumentList @("ls-remote", "--exit-code", "origin", "refs/heads/$branchName")

        if ($remoteCheckResult.ExitCode -eq 0) {
            $fetchResult = Invoke-GitCapture -ArgumentList @("fetch", "origin", "refs/heads/$branchName`:$branchName")
            if ($fetchResult.ExitCode -ne 0) {
                throw "Remote branch '$branchName' exists but could not be fetched."
            }
        } elseif ($remoteCheckResult.ExitCode -eq 2) {
            Write-Host "Remote branch '$branchName' not found, will create fresh."
        } else {
            throw "Could not verify whether remote branch '$branchName' exists. Check git remote/authentication. $($remoteCheckResult.StdErr)"
        }

        $branchListResult = Invoke-GitCapture -ArgumentList @("branch", "--list", $branchName)
        if ($branchListResult.ExitCode -ne 0) {
            throw "Failed to inspect local branches after fetch: $($branchListResult.StdErr)"
        }
        $localBranchExists = [bool]$branchListResult.StdOut
    }

    if ($localBranchExists) {
        Write-Host "Creating worktree for role '$Role' from existing branch '$branchName'..."
        $addResult = Invoke-GitCapture -ArgumentList @("worktree", "add", "-f", $worktreePath, $branchName)
        if ($addResult.ExitCode -ne 0) {
            throw "Failed to create worktree for role '$Role': $($addResult.StdErr)"
        }
    } else {
        Write-Host "Creating worktree for role '$Role' (fresh branch from $BaseBranch)..."
        $addResult = Invoke-GitCapture -ArgumentList @("worktree", "add", "-f", $worktreePath, "-b", $branchName, $BaseBranch)
        if ($addResult.ExitCode -ne 0) {
            throw "Failed to create worktree for role '$Role': $($addResult.StdErr)"
        }
    }
}

New-AgentWorktree -Role "planner"
New-AgentWorktree -Role "implementer"
New-AgentWorktree -Role "tester"
New-AgentWorktree -Role "security-checker"
New-AgentWorktree -Role "reviewer"
New-AgentWorktree -Role "doc-checker"

Write-Host ""
Write-Host "Created agent worktrees under: $WorktreeRoot"
Write-Host "Use 'git worktree list' to verify."
