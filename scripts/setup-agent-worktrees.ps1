param(
    [string]$BaseBranch = "main",
    [string]$Prefix = "agent",
    [string]$WorktreeRoot = "..",
    [int]$IssueNumber = 0
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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
        $removeOut = git worktree remove --force $worktreePath 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Could not remove existing worktree at '$worktreePath': $removeOut"
        }
    }

    $localBranchExists = [bool](git branch --list $branchName)
    if (-not $localBranchExists) {
        # Fetch from remote if available, creating a local tracking branch
        git fetch origin "${branchName}:${branchName}" 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Remote branch '$branchName' not found, will create fresh."
        }
        $localBranchExists = [bool](git branch --list $branchName)
    }

    if ($localBranchExists) {
        Write-Host "Creating worktree for role '$Role' from existing branch '$branchName'..."
        git worktree add $worktreePath $branchName
    } else {
        Write-Host "Creating worktree for role '$Role' (fresh branch from $BaseBranch)..."
        git worktree add $worktreePath -b $branchName $BaseBranch
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
