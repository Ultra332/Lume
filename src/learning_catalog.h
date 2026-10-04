#ifndef LUME_LEARNING_CATALOG_H
#define LUME_LEARNING_CATALOG_H

#include "common.h"

typedef struct {
    const char *slug;
    const char *title;
    const char *definition;
    const char *example;
    const char *explanation;
    const char *uses;
    const char *related;
    const char *lesson;
} LearningConcept;

typedef struct {
    const char *slug;
    const char *title;
    const char *lume;
    const char *destination;
    const char *changed;
    const char *preserved;
} TransitionTopic;

typedef struct {
    const char *slug;
    const char *name;
    const char *introduction;
    const TransitionTopic *topics;
    size_t topic_count;
} TransitionLanguage;

size_t learning_concept_count(void);
const LearningConcept *learning_concept_at(size_t index);
const LearningConcept *learning_concept_find(const char *slug);
size_t transition_language_count(void);
const TransitionLanguage *transition_language_at(size_t index);
const TransitionLanguage *transition_language_find(const char *slug);

#endif
