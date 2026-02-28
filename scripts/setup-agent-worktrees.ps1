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
    if (git branch --list $branchName) {
        Write-Host "Deleting existing branch '$branchName'..."
        $deleteOut = git branch -D $branchName 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Could not delete existing branch '$branchName': $deleteOut"
        }
    }

    Write-Host "Creating worktree for role '$Role'..."
    git worktree add $worktreePath -b $branchName $BaseBranch
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
