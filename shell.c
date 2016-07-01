
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <signal.h>

#define LIM " \t\r\n\a"//Delimitadores
#define BUFFSIZE 128 //Cantidad de elementos.

/*Colores para printf*/
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\033[32;1m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\033[0m"

void loop(void);
int cd(char **args);
int help(char **args);
int eexit(char **args);
int source(char **args);
int eexport(char **args);
char* ultimoArgumento(char **args);
int comprobarEjecucionSegundoPlano(char **args);
int contieneTarget(char *args,char target);
char** parseTarget(char **args,char *target);
void ponerPid(pid_t pid);
void reaper(int s);
int jobs();
char** activarRedireccion(char **args);
void desactivarRedireccion();
char** argumentosSinAnd(char** args);
void quitarPid(pid_t pid);
void sigproc();
int comprobarProcesosLleno();

static pid_t procesos[BUFFSIZE];
static pid_t foreGround = 0;    




/*
 Lista de los Strings de comandos
 */
char *nombresComandos[] = {
    "cd",
    "help",
    "exit",
    "source",
    "export",
    "jobs"
};

/*
*Direcciones de las funciones de los comandos
*/
int (*direccionesFunciones[]) (char **) = {
    &cd,
    &help,
    &eexit,
    &source,
    &eexport,
    &jobs
};


/*
*En el main unicamente ejecutamos un loop de la consola.
*/
int main(){
    signal(SIGCHLD, reaper);//Señal del reaper
    signal(SIGINT, sigproc); /* Ctrl+C */
    loop();
    return 1;
}

/*
@return  Longitud de la array de los Strings de los comandos.
*/
int tamNombresComandos(){
    return sizeof(nombresComandos) / sizeof(char *);
}

/*
 @resumen Crea proceso hijo y espera a que acabe.
 @parametros args Lista de los argumentos acabada en elemento nulo.
 @return 1 Devuelve '1' para que el loop de la consola siga ejecutandose
 */
int lanzar(char **args,int segundoPlano){
    pid_t pid, wpid;
    int status;
    
    pid = fork();    

    switch(pid){

        case -1:
            perror("Error en el forking");
            break;

        case 0://El caso del hijo
            if(segundoPlano){
				signal(SIGINT,SIG_IGN);

   			}
            else{
                signal(SIGINT,SIG_DFL);
            }

           
            if (execvp(args[0], args) == -1) {//El hijo ejecuta y comprobamos si ha ido bien
            	perror("BASH: Error en la ejecución del hijo");
            
            exit(EXIT_FAILURE);
            }


            break;

        default://En el caso que sea el padre esperará a que acabe el hijo.
            if(segundoPlano==1){
            ponerPid(pid);
        }
        
        else{
            foreGround=pid;//Actualizamos el proceso que se esta ejecutando en foreGround
        
        do {

	        wpid = waitpid(pid, &status, WUNTRACED);
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));
        
    	}
        break;
    }
    
    return 1;
}


/*
 @resumen Ejecuta las funciones de los comandos de la consola
 @parametros args Lista de los argumentos acabada en elemento nulo.
 @return 1 Devuelve '1' para que el loop de la consola siga ejecutandose
 */
int ejecutar(char **args){
    int segundoPlano,i=0;
    if (args[0] == NULL) { //En este caso no hemos introducido ningun comando y salimos.
        
        return 1;
    }
    activarRedireccion(args);//En el caso que haya redireccion se trata

    if(comprobarEjecucionSegundoPlano(args)){

        segundoPlano=1;//Modificamos el 'flag' que pasamos por parametro para lanzar una funcion del sistema
    }
    if(comprobarProcesosLleno() && comprobarEjecucionSegundoPlano(args)){//En el caso de que este la lista de procesos llena y ejecutemos en segundo plano salimos.
        printf("Lista lista de procesos esta llena\n");
        return 1;
    }
    for (i = 0; i < tamNombresComandos(); i++) {

        if (strcmp(args[0], nombresComandos[i]) == 0) {
            return (*direccionesFunciones[i])(args);//Ejecutamos las funciones propias pasando los argumentos
        }
    }
   
    return lanzar(argumentosSinAnd(args),segundoPlano);//Lanzamos las funciones del sistema
}

/*
 @resumen Lee una linea del teclado (stdin).
 @parametros void.
 @return Linea leida de
 */
char *leerLinea(void){
    char* entrada = readline(ANSI_COLOR_GREEN"BASH:/>"ANSI_COLOR_RESET);
    add_history(entrada);
    return entrada;
}

 /*
 @resumen Separa la linea en tokens.
 @parametros Linea que separa la funcion.
 @return Array de tokens acabada en caracter nulo
 */
char **separarLinea(char *line){
    int position = 0;
    char **tokens = malloc(BUFFSIZE * sizeof(char*));
    char *token;
    
    if (!tokens) {
        printf("Error con los tokens\n");
        exit(EXIT_FAILURE);
    }
    
    token = strtok(line, LIM);

    while (token != NULL) {
        tokens[position] = token;
        position++;   
        
        token = strtok('\0',LIM);
    }
    tokens[position] = NULL;
    return tokens;
}


/*
 @resumen loop de la consola.
 @parametros void.
 */
void loop(void){
    char *line;
    char **args;
    int status;
    
    do {
        line = leerLinea();
        if(line!=NULL){//Si la linea es nula hemos introducido "CTRL+D"  y devemos finalizar la ejecucion
            args = separarLinea(line);
            status = ejecutar(args);

            free(line);
            free(args);
        }
        else{
            printf("\n");
            status=0;//Finalizamos la ejecucion
        }       

        desactivarRedireccion();//Aseguramos que stdout siempre apunta a la consola
    } while (status);
}

/***************************************************************************************************************
                              DEFINICION DE TODOS LOS COMANDOS PROPIOS
****************************************************************************************************************/


/*
 @resumen Cambia de directorio.
 @parametros args Lista de los argumentos acabada en elemento nulo. arg[1] es el directorio
 @return 1 Devuelve '1' para que el loop de la consola siga ejecutandose.
*/
int cd(char **args){

    if (args[1] == NULL) { //No hay argumentos      
        printf("BASH: Argumentos no validos\n");
    } 

    if (chdir(args[1]) != 0) { //El directorio introducido no existe
        perror("BASH");
    }
    return 1;
}

/*
 @resumen Imprime la ayuda.
 @parametros args Lista de los argumentos acabada en elemento nulo.
 @return 1 Devuelve '1' para que el loop de la consola siga ejecutandose.
 */
int help(char **args){
    int i;
    printf("BASH 1.0V\n");    
    printf("Hola %s !\n",getenv("USER") );//Imprimimos el usuario
    printf("Los comandos soportados son los siguientes:\n");

    for (i = 0; i < tamNombresComandos(); i++) {
        printf( ANSI_COLOR_BLUE"%s" ANSI_COLOR_RESET"\n", nombresComandos[i]);//Añadimos color azul a la lista de los comandos
    }  

    printf("Ademas de estos comandos puedes utilizar los del sistema\n");  

    return 1;
}

/*
 @resumen Finaliza la ejecución devolviendo '0'.
 @parametros args Lista de los argumentos acabada en elemento nulo.
 @return Devuelve '1' para que el loop de la consola siga ejecutandose.
*/
int eexit(char **args){
    return 0;
}

/*
 @resumen Ejecuta comandos desde un source.
 @parametros args Lista de los argumentos acabada en elemento nulo.
 @return '1' Continua el loop de la consola.
*/
int source(char **args){
   
    int status;
   
    FILE *f = fopen(args[1],"r");//Creamos el fichero para leer.
   
    if (f!=NULL){

        char *linea = malloc(sizeof(char)*BUFFSIZE); 

        while (fgets(linea, sizeof(char)*BUFFSIZE, f)) {   
           char **argumentos;  

            /*Tratamos las lineas que hemos obtenido del fichero*/  
                                 
            argumentos = separarLinea(linea);
            status = ejecutar(argumentos);  
            free(argumentos);
           
        }

        free(linea);      

        fclose(f);
    }else{
        printf("BASH: El fichero no existe\n");
    }
   
    return 1 ;
}

/*
 @resumen Imprime la lista de procesos en segundo plano.
 @parametros void.
 @return 1 Devuelve '1' para que el loop de la consola siga ejecutandose.
 */
int jobs(){
    int j = 0;
    while(procesos[j]!='\0'){
        printf("[%d]-----PID: %d\n",j,procesos[j] );
        j++;
    }

    if(j==0){
        printf(ANSI_COLOR_RED"No Hay procesos en BackGround"ANSI_COLOR_RESET"\n");
    }
    return 1;
}

/*
 @resumen Añade una variable de entorno.
 @parametros args Lista de los argumentos acabada en elemento nulo.
 @return 1 Devuelve '1' para que el loop de la consola siga ejecutandose.
 */
int eexport(char **args){
    int overWrite = 0;
    char** tokens = parseTarget(args,"=");

  	if(getenv(tokens[0])!=NULL){//Si la variable de entorno existe, hay que sobre escribirla
        overWrite=1;
    }

    if(setenv(tokens[0],tokens[1],overWrite)){
        perror("BASH: error al asignar la variable de entorno");
    }
    free(tokens);
    return 1;
}


/***************************************************************************************************************
                              DEFINICION DE FUNCIONES NECESARIAS PARA LA IMPLEMENTACION
****************************************************************************************************************/
/*
 @resumen Si los argumentos cumplen las condiciones activa la redireccion de stdout al fichero asignado.
 @parametros args Lista de los argumentos acabada en elemento nulo.
 @return Devuelve los tokens, el primero sera el comando y el segundo el fichero donde se redirecciona la salida.
 */
char** activarRedireccion(char **args){
  	if(contieneTarget(ultimoArgumento(args),'>')){
        char** tokens = parseTarget(args,">");   
	    if(tokens[0]!=NULL&&tokens[1]!=NULL){
    		freopen (tokens[1],"w",stdout);
    	  	return tokens;
	    }
        free(tokens);
	}
   return NULL;
}

/*
 @resumen Redirecciona stdout a la consola. Lo ejecutaremos en el loop inicial para asegurarnos 
 *que al final de cada orden stdio apunta a la consola. 
 @parametros void.
 @return void.
 */
void desactivarRedireccion(){
    freopen ("/dev/tty", "a", stdout);
}

/*
 @resumen Modifica el ultimo argumento para que pueda ser tratado.
 @parametros args Lista de los argumentos acabada en elemento nulo. target Elemento por el cual queremos cortar el argumento.
 @return 1 Devuelve los tokens generados al cortar por el target.
 */
char** parseTarget(char **args,char *target){
    int position = 0;
    char **tokens = malloc(BUFFSIZE * sizeof(char*));
    char *token;
    
    if (!tokens) {
        printf("Error con los tokens\n");
        exit(EXIT_FAILURE);
    }
    
    token = strtok(ultimoArgumento(args),target);
    while (token != NULL) {
        tokens[position] = token;
        position++;   
        token = strtok('\0',"LIM");
    }
    tokens[position] = NULL;
    free(token);
    return tokens;
}

/*
 @resumen Comprueba si hay ejecucion en segundo plano.
 @parametros args Lista de los argumentos acabada en elemento nulo.
 @return 1 Devuelve '1' si hay ejecucion en segundo plano.
 */
int comprobarEjecucionSegundoPlano(char **args){
    if(contieneTarget(ultimoArgumento(args),'&')){
        return 1;
    }
    return 0;
}

/*
 @resumen Comprueba si hay redireccion a un fichero.
 @parametros args Lista de los argumentos acabada en elemento nulo.
 @return 1 Devuelve '1' si hay redireccion a un fichero.
 */
int comprobarRedireccionFichero(char **args){
    if(contieneTarget(ultimoArgumento(args),'>')){
        return 1;
    }
    return 0;
}

/*
 @resumen Obtiene el ultimo argumento.
 @parametros args Lista de los argumentos acabada en elemento nulo.
 @return 1 Devuelve el ultimo argumento.
 */
char* ultimoArgumento(char **args){
    int indice = 1;
    while(args[indice]!=NULL){

        indice++;
    }
    return args[indice-1];
}

/*
 @resumen Comprueba si un argumento tiene un target.
 @parametros args Argumento que queremos comprobar.
 @return 1 Devuelve '1' si el argumento contiene el target.
 */
int contieneTarget(char *args,char target){
    int indice = 0;
    while(args[indice]!='\0'){
        if(args[indice]==target){
            return 1;
        }
        indice++;
    }

    return 0;
}

/*
 @resumen Añade un pid a la lista de procesos en background.
 @parametros pid Pid que queremos añadir
 @return void.
 */
void ponerPid(pid_t pid){
    int i = 0;
    while(procesos[i]!='\0' ){
        i++;
    }
    procesos[i]=pid;
}

/*
 @resumen Cuando un pid ha acabo lo quita de la array de procesos.
 @parametros s
 @return void.
 */
void reaper(int s){
    pid_t pid;

	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {            
	    quitarPid(pid);
	}      
}

/*
 @resumen Devuelve los argumentos sin la & final.
 @parametros Lista de los argumentos acabada en elemento nulo.
 @return args Devuelve la lista de argumentos sin &.
 */
char** argumentosSinAnd(char** args){
    int i = 0;    
    while(args[i]!=NULL){
        i++;
    }
  
    if(comprobarEjecucionSegundoPlano(args)){
  
        args[i-1]=NULL;
    }
    return args;
}

/*
 @resumen Quita el pid que pasamos por parametro de la lista de procesos.
 @parametros pid  Pid que queremos quitar de la lista de procesos.
 @return void.
 */
void quitarPid(pid_t pid){   

	int posTarget = 0;

	while(procesos[posTarget]!=pid){
	    posTarget++;
	}

	int posFinal = posTarget;

	while(procesos[posFinal]!='\0'){
	    posFinal++;
	}

	posFinal--;

	procesos[posTarget]=procesos[posFinal];
	procesos[posFinal]='\0';
}

/*
 @resumen funcion que ejecuta la señal sigint. Mata el proceso que se esta ejecutando en foreGround
 @parametros void
 @return void.
 */
void sigproc(){
    if(foreGround!=0){
        kill(foreGround, SIGKILL); 
    }    
}

int comprobarProcesosLleno(){

    int indice = 0;

    while(procesos[indice]!='\0'){

        indice++;
    }

    if(indice==BUFFSIZE){

        return 1;
    }

    return 0;

}