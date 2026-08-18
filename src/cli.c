#include "cli.h"
#include <string.h>
#include "analyzer.h"
#include "common.h"
#include "diagnostic.h"
#include "education.h"
#include "lexer.h"
#include "memory.h"
#include "repl.h"
#include "session.h"
#include "parser.h"
#include "project.h"
#include "dependency.h"
static void warn_stale_lock(const DependencyGraph *graph,RuntimeIO io){bool exists=false;if(!dependency_lock_current(graph,&exists)&&exists)fputs("Aviso: lume.lock esta desatualizado. Execute 'lume resolver'.\n",io.output);}
static void help(FILE *out) { fputs("Lume - linguagem de programacao educacional\n\nUso:\n  lume arquivo.lume       Executa um programa\n  lume novo <nome>        Cria um projeto\n  lume executar [projeto] Executa um projeto ou arquivo\n  lume verificar [projeto] Valida sem executar\n  lume resolver [projeto]  Gera o lockfile local\n  lume                    Abre o modo interativo\n  lume --ajuda            Mostra esta ajuda\n  lume --versao           Mostra a versao\n  lume --tokens arquivo   Mostra os tokens\n  lume --expr \"expressao\" Avalia uma expressao\n  lume --analisar arquivo Analisa sem executar\n  lume --explicar arquivo Explica a execucao em portugues\n  lume --passo arquivo    Executa um evento por vez\n", out); }
static int run_text(const char *name, const char *text, size_t length, RuntimeIO io, bool expression) {
    LumeSession session; ErrorList errors; Source *source = NULL; bool ok; session_init(&session, io); error_list_init(&errors);
    ok = session_execute(&session, name, text, length, expression, &source, &errors);
    if (!ok && errors.count > 0U) diagnostic_render(io.output, source, &errors.data[0]);
    if (!ok && source != NULL) { source_free(source); memory_free(source); } error_list_free(&errors); session_free(&session); return ok ? 0 : 1;
}
static int run_file(const char *path, RuntimeIO io) { Source source; int result; source_init(&source);
    if (!source_load_file(&source, path)) { fprintf(io.output, "Nao foi possivel ler '%s'.\n", path); return 1; }
    result = run_text(path, source.bytes, source.length, io, false); source_free(&source); return result; }
static int run_project(const char *root,RuntimeIO io){DependencyGraph graph;const LumeProject *p;Source source;LumeSession session;ErrorList errors;Source *error_source=NULL;bool ok;dependency_graph_init(&graph);if(!dependency_graph_resolve(&graph,root)){fprintf(io.output,"%s\n",graph.message);dependency_graph_free(&graph);return 1;}warn_stale_lock(&graph,io);p=dependency_graph_root(&graph);source_init(&source);if(!source_load_file(&source,p->entry_path)){dependency_graph_free(&graph);return 1;}session_init(&session,io);module_registry_set_project(&session.modules,p->source_path,p->module_paths,p->module_path_count);module_registry_set_dependencies(&session.modules,&graph);error_list_init(&errors);ok=session_execute(&session,p->entry_path,source.bytes,source.length,false,&error_source,&errors);if(!ok&&errors.count>0U)diagnostic_render(io.output,error_source==NULL?&source:error_source,&errors.data[0]);if(!ok&&error_source!=NULL&&error_source!=&source){source_free(error_source);memory_free(error_source);}error_list_free(&errors);session_free(&session);source_free(&source);dependency_graph_free(&graph);return ok?0:1;}
static int tokens(const char *path, RuntimeIO io) { Source source; TokenArray array; ErrorList errors; bool ok; size_t index;
    source_init(&source); token_array_init(&array); error_list_init(&errors); ok = source_load_file(&source, path); if (ok) ok = lexer_scan(&source, &array, &errors);
    if (!ok && errors.count > 0U) diagnostic_render(io.output, &source, &errors.data[0]);
    if (ok) for (index=0U; index<array.count; index++) { fprintf(io.output,"%-18s %zu:%zu  ",token_type_name(array.data[index].type),array.data[index].span.start.line,array.data[index].span.start.column); fwrite(token_lexeme(&array.data[index]),1U,token_length(&array.data[index]),io.output); fputc('\n',io.output); }
    error_list_free(&errors); token_array_free(&array); source_free(&source); return ok ? 0 : 1; }
static int analyze_file(const char *path,RuntimeIO io){Source source;TokenArray tokens_array;ErrorList errors;Program *program=NULL;AnalysisResult analysis;ModuleRegistry registry;bool ok;int result=1;source_init(&source);token_array_init(&tokens_array);error_list_init(&errors);analysis_result_init(&analysis);module_registry_init(&registry,NULL,NULL);ok=source_load_file(&source,path);if(ok)ok=lexer_scan(&source,&tokens_array,&errors);if(ok)ok=parser_parse_program(&tokens_array,&program,&errors);if(ok)ok=module_registry_validate_program(&registry,path,&source,program,&errors);if(ok)ok=analyzer_analyze(program,&analysis);if(ok){analyzer_render(io.output,&source,&analysis);result=analysis.errors==0U?0:1;}else if(errors.count>0U)diagnostic_render(io.output,registry.error_source==NULL?&source:registry.error_source,&errors.data[0]);else fprintf(io.output,"Nao foi possivel analisar '%s'.\n",path);module_registry_free(&registry);analysis_result_free(&analysis);program_free(program);error_list_free(&errors);token_array_free(&tokens_array);source_free(&source);return result;}
static int verify_project(const char *root,RuntimeIO io){DependencyGraph graph;const LumeProject *p;Source source;TokenArray ta;ErrorList errors;Program *program=NULL;AnalysisResult analysis;ModuleRegistry registry;bool ok;int result=1;dependency_graph_init(&graph);if(!dependency_graph_resolve(&graph,root)){fprintf(io.output,"%s\n",graph.message);dependency_graph_free(&graph);return 1;}warn_stale_lock(&graph,io);p=dependency_graph_root(&graph);source_init(&source);token_array_init(&ta);error_list_init(&errors);analysis_result_init(&analysis);module_registry_init(&registry,NULL,NULL);module_registry_set_project(&registry,p->source_path,p->module_paths,p->module_path_count);module_registry_set_dependencies(&registry,&graph);ok=source_load_file(&source,p->entry_path);if(ok)ok=lexer_scan(&source,&ta,&errors);if(ok)ok=parser_parse_program(&ta,&program,&errors);if(ok)ok=module_registry_validate_program(&registry,p->entry_path,&source,program,&errors);if(ok)ok=analyzer_analyze(program,&analysis);if(ok){fprintf(io.output,"Verificando projeto %s (%s)\n",p->name,p->version);analyzer_render(io.output,&source,&analysis);result=analysis.errors==0U?0:1;}else if(errors.count>0U)diagnostic_render(io.output,registry.error_source==NULL?&source:registry.error_source,&errors.data[0]);module_registry_free(&registry);analysis_result_free(&analysis);program_free(program);error_list_free(&errors);token_array_free(&ta);source_free(&source);dependency_graph_free(&graph);return result;}
static int resolve_project(const char *root,RuntimeIO io){DependencyGraph graph;char message[512];bool ok;dependency_graph_init(&graph);fputs("Resolvendo dependencias...\n",io.output);ok=dependency_graph_resolve(&graph,root);if(!ok)fprintf(io.output,"%s\n",graph.message);else{ok=dependency_lock_write(&graph,message,sizeof(message));fprintf(io.output,"%s\n",message);}dependency_graph_free(&graph);return ok?0:1;}
int cli_run(int argc, char **argv, RuntimeIO io) {
    if (argc==1) return repl_run(io);
    if (argc==2 && (strcmp(argv[1],"--ajuda")==0 || strcmp(argv[1],"--help")==0)) { help(io.output); return 0; }
    if (argc==2 && (strcmp(argv[1],"--versao")==0 || strcmp(argv[1],"--version")==0)) { fprintf(io.output,"Lume %s\n",LUME_VERSION_STRING); return 0; }
    if (argc==3 && strcmp(argv[1],"--run")==0) return run_file(argv[2],io);
    if (argc==3 && strcmp(argv[1],"--tokens")==0) return tokens(argv[2],io);
    if (argc==3 && strcmp(argv[1],"--expr")==0) return run_text("<expressao>",argv[2],strlen(argv[2]),io,true);
    if (argc==3 && strcmp(argv[1],"--analisar")==0) return analyze_file(argv[2],io);
    if (argc==3 && strcmp(argv[1],"--explicar")==0) return education_run_file(argv[2],io,EDUCATION_EXPLAIN);
    if (argc==3 && strcmp(argv[1],"--passo")==0) return education_run_file(argv[2],io,EDUCATION_STEP);
    if (argc==3 && strcmp(argv[1],"novo")==0){char message[512];bool ok=project_create(".",argv[2],message,sizeof(message));fprintf(io.output,"%s\n",message);return ok?0:1;}
    if ((argc==2||argc==3)&&strcmp(argv[1],"executar")==0){const char *path=argc==3?argv[2]:".";size_t n=strlen(path);return n>=5U&&strcmp(path+n-5U,".lume")==0?run_file(path,io):run_project(path,io);}
    if ((argc==2||argc==3)&&strcmp(argv[1],"verificar")==0)return verify_project(argc==3?argv[2]:".",io);
    if ((argc==2||argc==3)&&strcmp(argv[1],"resolver")==0)return resolve_project(argc==3?argv[2]:".",io);
    if (argc==2 && argv[1][0]!='-') return run_file(argv[1],io);
    if (argc>=2 && argv[1][0]=='-') fprintf(io.output,"Opcao desconhecida: %s\nUse 'lume --ajuda' para ver os comandos disponiveis.\n",argv[1]); else fputs("Uso invalido. Use 'lume --ajuda'.\n",io.output);
    return 2;
}
