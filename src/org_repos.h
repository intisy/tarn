#ifndef TARN_ORG_REPOS_H
#define TARN_ORG_REPOS_H

#include "context.h"
#include "string_list.h"

/** The lines an org-repos block holds, built from the snapshot, with the text of each kept by repo name. */
void org_repos_lines(const check_context *context, const string_list *current_lines, string_list *out);
/**
 * Whether the block lists the repo: every repo of the checked repo's owner but itself, and only the public ones when
 * the checked repo is public, so a public block never names a private repo.
 */
int org_repos_is_listed(const check_context *context, const snapshot_repo *repo);
/** Tells whether the line reads `- [<owner>/<name>](https://github.com/<owner>/<name>) - <text>`. */
int org_repos_is_line(const char *line);
int org_repos_is_unwritten(const char *line);
/** The `<owner>/<name>` a well-formed line lists, as a new string; NULL for a line in another form. */
char *org_repos_line_name(const char *line);

#endif
