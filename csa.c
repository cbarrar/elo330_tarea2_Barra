#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int points=320;// 40ms/0.125ms=320, 0.125ms = 1/8KHz
int MAX =32767;//0x7FFF
int MIN=-32768;
int size=0; //cantidad de Bytes en el archivo.raw
short *buff; //arreglo con los valores del .raw
short *sat; //arreglo con valores saturados
short *new; //arreglo con valores recuperados

char *file;
int gain=0;
int offset=0;
char *p=NULL;

/* Read file.raw and copy the content to dynamic array */
void original_array(char *file){
	short sample;
	char caracter;
	int fd;
	
	if ( (fd = open(file, O_RDONLY)) < 0) {
		fprintf(stderr,"open failed \n");
	exit(1);
	}
	int i=0;
	//Size is the file'size
	while (read(fd, &sample,sizeof(sample)) >0)
		size++;
	//assign memory
	buff = malloc(size*2);
	//returns the pointer to the beginning
	lseek(fd, 0, SEEK_SET);
	// read fd and save the content in buff
	while (read(fd, &sample,sizeof(sample)) >0)
		buff[i++]=sample;
	close(fd);			
}
/* creates the saturated array */
void saturated_array(){
	int i=0;
	sat		= malloc(size*2);
	while(i<=size){
		if(buff[i]*gain > MAX )
			sat[i]=MAX;
		else if(buff[i]*gain < MIN )
			sat[i]=MIN;
		else
			sat[i]=buff[i]*gain;
		i++;
	}
}

/* creates a file using name and data*/
void create_file(char *name, short *data){
	int fd,i=0;
	short temp;
	
	if ( (fd = open(name, O_WRONLY|O_CREAT, 0666)) < 0) {
		fprintf(stderr,"open failed \n");
		exit(1);
	}
	while(i<=size){
		temp=data[i];
		if(write(fd, &temp,sizeof(temp))<0)
			fprintf(stderr,"write failed \n");
		i++;
	}
	close(fd);	
}
/* communicates with octave and send two vectors */
void interpolation(int y0, int y1, int y2, int y3, int init, int end){
	char *temp_file = "temp_file";
	char command[300], num[7];
	char to_eval[1000];
	int i;
	short int x[4]={init,init+1,end-1,end};
	to_eval[0] = '\0';
	for(i=init; i<=end; i++){
		if(i!=end)
			sprintf(num, "%d,",i);
		else
			sprintf(num, "%d", i);
		strcat(to_eval, num);
	}
	sprintf(command, "octave -qi --eval \"p=polyfit([%d,%d,%d,%d],[%d,%d,%d,%d],%d);RESULT=polyval(p,[%s]);csvwrite('%s',RESULT)\""
	, x[0], x[1], x[2], x[3], y0, y1, y2, y3, 4, to_eval, temp_file);
    system(command);
	FILE* output = fopen(temp_file, "r");
	if (output == NULL) {
		fprintf(stderr, "Error: couldn't open file %s\n",temp_file);
		exit(-1);
	}
	float result[end- init];
	int k;
	for(k=0; k<=(end-init); k++){
		if(k == end - init)
		fscanf(output, "%f", &(result[k]));
		else
		fscanf(output, "%f,", &(result[k]));
	}
	
	for(k=0,i=init; k<=(end-init) &&  i<=end ;k++,i++){
		new[i]=(short)result[k];
		if(new[i]>=MAX/gain)
			new[i]=MAX/gain;
		else if(new[i]>=MIN/gain)
			new[i]=MIN/gain;		
	}
	fclose(output);
}	
/* creates the array unsaturated */
void new_array(){
	int i=0;
	new	= malloc(size*2);
	while(i<=size){
		new[i]=sat[i]/gain;
		i++;
	}
}

/* find saturated values and send to interpolation */
void find_MAXMIN(){
	int i=0,init,end;
	short pol_y[4]={0,0,0,0}; 
					/*	pol_y[0] before the saturation point
						por_y[1]=pol_y[2] contains a saturated point
						pol_y[3] after the saturation point
					*/
	if(new[0]==MAX/gain)		
		pol_y[0]=MAX/gain;
	if(new[size]==MIN/gain)
		pol_y[size]=MIN/gain;
		
	for(i=0;i<=size;i++){
		if((new[i]==MAX/gain || sat[i]==MIN/gain)){
			init=i;
			pol_y[0]=new[i-1];
			pol_y[1]=new[i];
			i++;
			while((new[i]==MAX/gain || new[i]==MIN/gain) && (i<=size)){
				i++;
			}
			pol_y[2]=new[i-1];
			pol_y[3]=new[i];
			end=i;
			if(pol_y[3]!=0){
				//printf("%d, %d, %d, %d\n",pol_y[0],pol_y[1],pol_y[2],pol_y[3]);
				interpolation(pol_y[0],pol_y[1],pol_y[2],pol_y[3],init,end);
				pol_y[0]=new[end];
				pol_y[1]=0;
				pol_y[2]=0;
				pol_y[3]=0;
			}
			
		}
	}
	
}
/* communicates with octave using pipes and make a audio graphic */
void make_graphic(short *x, int pos, int init_oct, int close_oct){
	pid_t pid;
    int pfd[2];
    int i, status;
    FILE *sd;
    
    if(init_oct){
		/* Create a pipe */
		if (pipe(pfd) < 0) {
			perror("pipe");
			exit(1);
		}
		/* Create a child process.*/
		if ((pid = fork()) < 0) {
			perror("fork");
			exit(1);
		}

		/* The child process executes "octave" with -i option.*/
		if (pid == 0) {
			int junk;
      
			/* Attach standard input of this child process to read from the pipe.*/
			dup2(pfd[0], 0);
			close(pfd[1]);  /* close the write end of the pipe */
			junk = open("/dev/null", O_WRONLY);
			dup2(junk, 1);  /* throw away any message sent to screen*/
			dup2(junk, 2);  /* throw away any error message */
			execlp("octave", "octave", "-qi", (char *) NULL);
			perror("exec");
			exit(-1);
		}

		/* We won't be reading from the pipe.*/
		close(pfd[0]);
    }
    
    
    sd = fdopen(pfd[1], "w");  /* to use fprintf instead of just write */
    fprintf(sd, "x= [");
    for ( i= offset; i<(points+offset); i++)
		fprintf(sd, "%d ", x[i]);
    fprintf(sd, "];\n"); 
    fprintf(sd, "y= [");
    for ( i= offset; i<(points+offset); i++){
		int offset_time= 0.125*i+offset;
		fprintf(sd, "%d ", offset_time);  /* other data could be sent here */
	}
    fprintf(sd, "];\n"); fflush(sd);
    fprintf(sd, "subplot(2,2,%d);\n",pos);
    fprintf(sd, "plot(y,x);\n"); fflush(sd);
    
    
    if(close_oct){
		sleep(10);
		fprintf(sd, "\n exit\n"); fflush(sd);
	
		/* Wait for the child to exit and close de pipe.*/
		waitpid(pid, &status, 0);
		fclose(sd);
	}
}

int main(int argc, char * argv[])
{
	int i=0,j;
	char name_file[40];
	char name_sat[40];
	char name_new[40];
	char *ext_sa="_sat.raw";
	char *ext_new="_new.raw";
	/* Validación de argumentos */
	if (argc < (3+1)) {
		printf("argumentos validos: <archivo_de_audio_original> <ganancia> <offset> [p]\n");
		printf("cantidad ingresada: %d \n", argc-1);
        return 1;
    } 
    else{
		file=argv[1];
		gain=atoi(argv[2]);
		offset=atoi(argv[3]);
		p=argv[4];
		if(gain==0){
			printf("La ganacia no puede ser 0\n");
			exit(1);
		}
		sprintf(name_file,"%s",file);
		while(file[i]!='.'){
			i++;
		}
		j=i;
		while(*ext_sa!='.'){
			file[i]=*ext_sa;
			ext_sa++;
			i++;
		}
		strncpy(name_sat, file, 40);
		sprintf(name_sat,"%s.raw",name_sat);
		while(*ext_new!='.'){
			file[j]=*ext_new;
			ext_new++;
			j++;
		}
		strncpy(name_new, file, 40);
		sprintf(name_new,"%s.raw",name_new);
		
		original_array(name_file);
		if(points+offset>size){
			fprintf(stderr,"el offset ingresado no permite graficar 40[ms] \n");
		}
		saturated_array();
		new_array();
		find_MAXMIN();
		create_file(name_sat,sat);
		create_file(name_new,new);
		
		make_graphic(buff,1,1,0);	
		make_graphic(sat,2,0,0);
		make_graphic(new,3,0,1);	
		
		//play file original, saturated and recovered
		if(p!=NULL){
			char aplay[70],aplay_sat[70],aplay_new[70];
			sprintf(aplay,"aplay --format=S16_LE -t raw %s",name_file);
			strcpy(aplay,aplay);
			sprintf(aplay_sat,"aplay --format=S16_LE -t raw %s",name_sat);
			strcpy(aplay_sat,aplay_sat);
			sprintf(aplay_new,"aplay --format=S16_LE -t raw %s",name_new);
			strcpy(aplay_new,aplay_new);
				printf("%s\n",aplay);
					printf("%s\n",aplay_sat);
					
			printf("%s\n",aplay_new);	
			system(aplay);
			system(aplay_sat);
			system(aplay_new);
			
					
		}
		//clear file saturated and recovered
		char delete_sat[70],delete_new[70];
		sprintf(delete_sat,"rm %s",name_sat);
		strcpy(delete_sat,delete_sat);
		sprintf(delete_new,"rm %s",name_new);
		strcpy(delete_new,delete_new);
		system(delete_sat);
		system(delete_new);
				
	}
    exit(0);
}
		
