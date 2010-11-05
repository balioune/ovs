/*
 * Copyright (c) 2009, 2010 Nicira Networks.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CLASSIFIER_H
#define CLASSIFIER_H 1

/* Flow classifier.
 *
 * A classifier is a "struct classifier",
 *      a hash map from a set of wildcards to a "struct cls_table",
 *              a hash map from fixed field values to "struct cls_rule",
 *                      which can contain a list of otherwise identical rules
 *                      with lower priorities.
 */

#include "flow.h"
#include "hmap.h"
#include "list.h"
#include "openflow/nicira-ext.h"
#include "openflow/openflow.h"

/* A flow classifier. */
struct classifier {
    int n_rules;                /* Total number of rules. */
    struct hmap tables;         /* Contains "struct cls_table"s.  */
};

/* A set of rules that all have the same fields wildcarded. */
struct cls_table {
    struct hmap_node hmap_node; /* Within struct classifier 'wctables'. */
    struct hmap rules;          /* Contains "struct cls_rule"s. */
    struct flow_wildcards wc;   /* Wildcards for fields. */
    int n_table_rules;          /* Number of rules, including duplicates. */
    int n_refs;                 /* Reference count used during iteration. */
};

/* A flow classification rule.
 *
 * Use one of the cls_rule_*() functions to initialize a cls_rule.
 *
 * The cls_rule_*() functions below maintain the following important
 * invariant that the classifier depends on:
 *
 *   - If a bit or a field is wildcarded in 'wc', then the corresponding bit or
 *     field in 'flow' is set to all-0-bits.  (The cls_rule_zero_wildcards()
 *     function can be used to restore this invariant after adding wildcards.)
 */
struct cls_rule {
    struct hmap_node hmap_node; /* Within struct cls_table 'rules'. */
    struct list list;           /* List of identical, lower-priority rules. */
    struct flow flow;           /* All field values. */
    struct flow_wildcards wc;   /* Wildcards for fields. */
    unsigned int priority;      /* Larger numbers are higher priorities. */
};

enum {
    CLS_INC_EXACT = 1 << 0,     /* Include exact-match flows? */
    CLS_INC_WILD = 1 << 1,      /* Include flows with wildcards? */
    CLS_INC_ALL = CLS_INC_EXACT | CLS_INC_WILD
};

void cls_rule_from_flow(const struct flow *, uint32_t wildcards,
                        unsigned int priority, struct cls_rule *);
void cls_rule_from_match(const struct ofp_match *, unsigned int priority,
                         int flow_format, uint64_t cookie, struct cls_rule *);

void cls_rule_zero_wildcards(struct cls_rule *);

char *cls_rule_to_string(const struct cls_rule *);
void cls_rule_print(const struct cls_rule *);

void classifier_init(struct classifier *);
void classifier_destroy(struct classifier *);
bool classifier_is_empty(const struct classifier *);
int classifier_count(const struct classifier *);
int classifier_count_exact(const struct classifier *);
struct cls_rule *classifier_insert(struct classifier *, struct cls_rule *);
void classifier_insert_exact(struct classifier *, struct cls_rule *);
void classifier_remove(struct classifier *, struct cls_rule *);
struct cls_rule *classifier_lookup(const struct classifier *,
                                   const struct flow *, int include);
bool classifier_rule_overlaps(const struct classifier *,
                              const struct cls_rule *);

typedef void cls_cb_func(struct cls_rule *, void *aux);

void classifier_for_each(const struct classifier *, int include,
                         cls_cb_func *, void *aux);
void classifier_for_each_match(const struct classifier *,
                               const struct cls_rule *,
                               int include, cls_cb_func *, void *aux);
struct cls_rule *classifier_find_rule_exactly(const struct classifier *,
                                              const struct cls_rule *);

/* Iteration shorthands. */

struct cls_table *classifier_exact_table(const struct classifier *);
struct cls_rule *cls_table_first_rule(const struct cls_table *);
struct cls_rule *cls_table_next_rule(const struct cls_table *,
                                     const struct cls_rule *);

#define CLS_TABLE_FOR_EACH_RULE(RULE, MEMBER, TABLE)                    \
    for ((RULE) = OBJECT_CONTAINING(cls_table_first_rule(TABLE),        \
                                    RULE, MEMBER);                      \
         &(RULE)->MEMBER != NULL;                                       \
         (RULE) = OBJECT_CONTAINING(cls_table_next_rule(TABLE,          \
                                                        &(RULE)->MEMBER), \
                                    RULE, MEMBER))

#define CLASSIFIER_FOR_EACH_EXACT_RULE(RULE, MEMBER, CLS)               \
    CLS_TABLE_FOR_EACH_RULE (RULE, MEMBER, classifier_exact_table(CLS))

#endif /* classifier.h */
