# Contributing to Postopek

First off, thank you for considering contributing to Postopek. It's people like you that make Postopek such a great tool.

## Where do I go from here?

If you've noticed a bug or have a feature request, make one! It's generally best if you get confirmation of your bug or approval for your feature request this way before starting to code.

## Fork & create a branch

If this is something you think you can fix, then fork and create a branch with a descriptive name.

A good branch name would be (where issue #325 is the ticket you're working on):

```sh
git checkout -b 325-condense-downloads-page
```

## Implement your fix or feature
At this point, you're ready to make your changes! Feel free to ask for help; everyone is a beginner at first.

## Get the style right
Your patch should follow the same conventions & pass the same code quality checks as the rest of the project.

## Make a Pull Request
At this point, you should switch back to your master branch and make sure it's up to date with the latest code from the main repository:

```sh
git remote add upstream git@github.com:pralad-p/Postopek.git
git checkout master
git pull upstream master
```

Then update your feature branch from your local copy of master, and push it!

```sh
git checkout 325-condense-downloads-page
git rebase master
git push --set-upstream origin 325-condense-downloads-page
```

Finally, go to GitHub and make a Pull Request 👍

## Keeping your Pull Request updated
If a maintainer asks you to "rebase" your PR, they're saying that a lot of code has changed, and that you need to update your branch so it's easier to merge.

To learn more about rebasing in Git, there are a lot of good resources but here's the suggested workflow:

```sh
git checkout 325-condense-downloads-page
git pull --rebase upstream master
git push --force-with-lease 325-condense-downloads-page
```

Merging a PR (maintainers only)
A PR can only be merged into master by a maintainer if:
- It is passing CI.
- It has no requested changes.
- It is up to date with current master.

Any maintainer is allowed to merge a PR if all of these conditions are met.
