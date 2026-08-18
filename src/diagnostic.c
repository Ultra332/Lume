#include "diagnostic.h"
#include <string.h>
void diagnostic_render(FILE *stream, const Source *source, const LumeError *error) {
    size_t line_start, line_end, marker_start, marker_length, index;
    if (stream == NULL || source == NULL || error == NULL) return;
    fprintf(stream, "%s:%zu:%zu\n\n", source->name, error->span.start.line, error->span.start.column);
    line_start = error->span.start.offset <= source->length ? error->span.start.offset : source->length;
    while (line_start > 0U && source->bytes[line_start - 1U] != '\n') line_start--;
    line_end = error->span.start.offset <= source->length ? error->span.start.offset : source->length;
    while (line_end < source->length && source->bytes[line_end] != '\n' && source->bytes[line_end] != '\r') line_end++;
    fwrite(source->bytes + line_start, 1U, line_end - line_start, stream); fputc('\n', stream);
    marker_start = error->span.start.offset >= line_start ? error->span.start.offset - line_start : 0U;
    for (index = 0U; index < marker_start && line_start + index < line_end; index++)
        fputc(source->bytes[line_start + index] == '\t' ? '\t' : ' ', stream);
    marker_length = error->span.end.offset > error->span.start.offset ?
        error->span.end.offset - error->span.start.offset : 1U;
    if (marker_length > line_end - (line_start + marker_start)) marker_length = line_end - (line_start + marker_start);
    if (marker_length == 0U) marker_length = 1U;
    for (index = 0U; index < marker_length; index++) fputc('^', stream);
    fprintf(stream, "\n\n%s:\n%s\n", error_kind_name(error->kind), error->message);
    if (error->subject != NULL) {
        fputs("\nNome: '", stream); fwrite(error->subject, 1U, error->subject_length, stream); fputs("'\n", stream);
    }
    if (error->suggestion != NULL) fprintf(stream, "\nDica:\n%s\n", error->suggestion);
}
