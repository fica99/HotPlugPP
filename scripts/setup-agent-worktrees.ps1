param(
    [string]$BaseBranch = "main",
    [string]$Prefix = "agent",
    [string]$WorktreeRoot = ".."
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function New-AgentWorktree {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Role
    )

    $branchName = "$Prefix/$Role/$BaseBranch"
    $worktreePath = Join-Path $WorktreeRoot ("HotPlugPP-" + $Role)

    Write-Host "Creating worktree for role '$Role'..."
    git worktree add $worktreePath -b $branchName $BaseBranch
}

New-AgentWorktree -Role "planner"
New-AgentWorktree -Role "implementer"
New-AgentWorktree -Role "tester"
New-AgentWorktree -Role "reviewer"
New-AgentWorktree -Role "fixer"

Write-Host ""
Write-Host "Created agent worktrees under: $WorktreeRoot"
Write-Host "Use 'git worktree list' to verify."
