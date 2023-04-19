# cppTango: How to contribute?

Thanks for your interest in contributing to cppTango. We welcome every contribution, regardless how small or big. You are spending your time on making cppTango better and that is fantastic!

In this document we try to explain how you will get your contribution into cppTango in the fastest possible way. But first we would like to lay out how important decisions are made in the cppTango team.

# From idea to code

In general it is a good idea, to discuss additions and changes first in an issue with the existing developers. This avoids duplicated work, disappointments and can save you a lot of time. Maybe something close to what you need is already there, and can be reused for your change.

We don't need a signed CLA (contributors license agreement) from you.

# Contribution overview

- Create an issue [here](https://gitlab.com/tango-controls/cppTango/-/issues/new) that describes the problem or the improvement. This helps us to track progress and link merge requests to the problems and improvements.
- Fork the repository to your own user.
- Add your fork as new remote: `git remote add myFork git@github.com:<user>/cppTango.git`
- Create a new branch for your work. Always branch off cppTango's `main` branch. Only if you are proposing a **critical bug fix** for the latest 9.3.x version, then branch off the `9.3-backports` branch.
- Start hacking.
- Test your modifications locally on your system. See the document [test/README](https://gitlab.com/tango-controls/cppTango/-/blob/main/test/README) for instructions.
    - if you don't have your own CI minutes, contact us when you want to test it on Gitlab.

- [Create a merge request](https://gitlab.com/tango-controls/cppTango/-/merge_requests/new) (MR). Begin the description of the MR with `Draft:` or `WIP:`. This indicates that you are still working on the MR.
- Rebase you code frequently! This will ensure that you do not face merge issues at the end of your work.
- Once the CI passes, mark the merge request as ready and remove `Draft:`or `WIP:`from the description. You have rebased your code, haven't you?
- Before an MR can be merged, the modifications need to be reviewed.
    - You can either assign Reviewers yourself, if you know who should review your MR,
    - or Reviewers will be assigned by the cppTango team.
    - Each MR needs one reviewer from the [Code Owners](https://gitlab.com/tango-controls/cppTango/-/blob/main/CODEOWNERS) (see [Approval](#approval) below).

- The reviewer will go through the modifications that you made and provide feedback.
- Once the review has concluded, the reviewer will approve the MR.
- After approval, the MR will be merged.

# Merge request acceptance and merging

Thank you very much for your work and congratulations! You have made cppTango better. 🎉

And now you want to get these changes merged? Very nice!

In order to make this process as smooth as possible for you, here are a few hints for the path forward:

- All CI tests have to pass. If you have changed the behaviour of the code, you should add new tests as well. You don't need to execute the tests locally, CI is the reference anyway. So just create a MR and let CI handle that.
- Make your MR easy to review. This starts with explaining what it wants to achieve and ends with splitting the changes into logical commits where each commit describes why it is changing the code.
- Try to keep the MR small. Nobody likes to review 500 changes in 30 files in a single MR.
- Please follow the coding style. This is at the moment messy at best, but still we don't want to get worse.
- Be prepared to adapt your pull request to the review responses. Code review is done for ensuring higher code quality and communicating implementations details to newcomers and not to annoy anybody or slowing down development.
- Adapting existing pull requests also involves force-pushing new versions as we don't want to have intermediate versions in the history.

# Approval

In general each merge request (MR) needs an approval from one of the [Code Owners](#glossary)
listed [here](https://gitlab.com/tango-controls/cppTango/-/blob/main/CODEOWNERS).  If the reviewing Code Owner deems the MR sufficiently complicated, they may ask for a review from someone else as well (perhaps not a code owner).


# Assigning an issue or an MR

Assignees of an issue or MR are usually the ones who are actively working on it. The cppTango team will assign the issues and MRs to the contributors and team members whether during the weekly meeting or when the open issues and MRs are groomed. If you, as contributor, think that somebody should get involved, leave a comment in the issue or MR and tag the comment with that person's user name.

## Issues

Please assign an issue to yourself if you are working on it. In that way everyone knows who is working on what.

## Merge Requests

- For the development phase, the creator will be assigned and the issue will be marked as draft.
- Once the MR is ready for review
  - The draft state needs to be removed from it.
  - It needs to be assigned to one of the [Code Owners](#glossary).
  - Then the cppTango team will handle the review.
- In most cases the reviewers will provide feedback, want something changed, etc. They will then assign the MR back to the merge request creator to reflect that.
  - Once the requested changes are implemented, the MR is assigned back to the reviewers.

If you have any questions, please get in touch!

# Issues and MRs with bigger impact

It might happen from time to time that an issue or an MR will have a significant impact on cppTango or even on Tango Controls altogether. These issues and MRs will be discussed by the cppTango team in their weekly meetings and on Tango Control's Slack channels. We are pretty good in finding a consensus as we are a group of dedicated software developers who have a strong interest in making Tango Controls a good piece of software with a solid foundation.

Very very rarely it happens though, that we disagree and cannot find a common ground. Then Reynald Bourtembourg will have the final say as he is cppTango's long time leader.

# Glossary

- [co]:[Code Owners](https://gitlab.com/tango-controls/cppTango/-/blob/main/CODEOWNERS): The cppTango contributors who have a little bit more decision power than contributors. Usually they are the people who are either paid for working on cppTango or Tango Controls or they can just spend a significant amount of their time on cppTango or Tango Controls.
