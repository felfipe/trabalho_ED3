#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include "funcoes_uteis.h"

#define TAM_REGISTRO 85
#define TAM_ESTADO 2


typedef struct{
    char estadoOrigem[3];
    char estadoDestino[3];
    int  distancia;
    char cidadeOrigem[40];
    char cidadeDestino[40];
    char tempoViagem[10];
} Route;

// Estrutura de cabeçario com 19 bytes
typedef struct{
    char status;
    int numero_vertices;
    int numero_arestas;
    char data_ultima_compactacao[11];
} Header;

FILE *open_file(char* file_name, char* mode){
    FILE *file;
    file = fopen(file_name,mode);
    if(file != NULL)
        return file;
    return NULL;
}

int close_file(FILE* file){
    if(file != NULL){
        fclose(file);
        return 1;
    }
    return -1;

}

void limpa_string(char* string, int tam){
    for(int i =0; i< tam;i++)
        string[i] = '\0';
}

void clear_route(Route *route){
    limpa_string(route->estadoOrigem,3);
    limpa_string(route->estadoDestino,3);
    limpa_string(route->cidadeOrigem,40);
    limpa_string(route->cidadeDestino,40);
    limpa_string(route->tempoViagem,10);
    route->distancia = 0;
}
void limpa_header(Header *head){
    limpa_string(head->data_ultima_compactacao,11);
    head->numero_arestas = 0;
    head->numero_vertices = 0;
    head->status = '0';
}
int write_header(Header head, FILE *file){
    fseek(file,0,SEEK_SET);
    fwrite(&(head.status),sizeof(char),1,file);
    fwrite(&(head.numero_vertices),sizeof(int),1,file);
    fwrite(&(head.numero_arestas),sizeof(int),1,file);
    fwrite(head.data_ultima_compactacao,10*sizeof(char),1,file);
    return 0;
}
int write_register(FILE* file, Route reg, int j){
	fwrite(reg.estadoOrigem,TAM_ESTADO*sizeof(char),1,file);
	fwrite(reg.estadoDestino,TAM_ESTADO*sizeof(char),1,file);
	fwrite(&(reg.distancia),sizeof(int),1,file);
	fwrite(reg.cidadeOrigem,strlen(reg.cidadeOrigem)*sizeof(char),1,file);
	fwrite("|",sizeof(char),1,file);
	fwrite(reg.cidadeDestino,strlen(reg.cidadeDestino)*sizeof(char),1,file);
	fwrite("|",sizeof(char),1,file);
	fwrite(reg.tempoViagem,strlen(reg.tempoViagem)*sizeof(char),1,file);
	fwrite("|",sizeof(char),1,file);
	int size = TAM_REGISTRO - 2*TAM_ESTADO - sizeof(int) - strlen(reg.cidadeOrigem) - strlen(reg.cidadeDestino) - strlen(reg.tempoViagem) - 3;
	if (j){
		for(int i = 0; i < size; i++)
			fwrite("#",sizeof(char),1,file);
	}
	return 0;
}
int query_city_file(FILE* cities, char *city){
    char query[40];
    fseek(cities,0,SEEK_SET);
    while(fgetc(cities) != EOF){
        fseek(cities,-1,SEEK_CUR);
        fread(query,40*sizeof(char),1,cities);
        if(!strcmp(city,query))
            return 0;
    }
    fwrite(city,40*sizeof(char),1,cities);
    return 1;
}
void get_current_date(char *date){
    time_t now;
    time(&now);
    struct tm *local = localtime(&now);
    char dia[3];
    char mes[3];
    if(local->tm_mday < 10)
        sprintf(dia,"0%d",local->tm_mday);
    else
        sprintf(dia,"%d",local->tm_mday);
    if(local->tm_mon+1 < 10)
        sprintf(mes,"0%d",local->tm_mon+1);
    else
        sprintf(mes,"%d",local->tm_mon+1);

    sprintf(date,"%s/%s/%4d",dia,mes,local->tm_year+1900);
}
int read_csv(char* csv_name, char* bin_name){
    FILE *csv_file = fopen(csv_name,"rb+");
    FILE *bin_file = fopen(bin_name,"wb");
    FILE *head_file = fopen("head.bin","wb+");
    if(csv_file == NULL || bin_file == NULL || head_file == NULL){
        printf("Falha no carregamento do arquivo.");
        return -1;
    }
    Route route;
    Header head;
    limpa_header(&head);
    //strcpy(head.data_ultima_compactacao,"00/00/0000");
    strcpy(head.data_ultima_compactacao,"##########");
    fwrite("#",sizeof(char),19,bin_file); // RESERVA 19 BYTES PARA CABEÇALHO
    char a = '\0';
    while(a != '\n'){
        a = fgetc(csv_file);
    }
    while(fgetc(csv_file) != EOF){
        char dist[10];
        fseek(csv_file,-1,SEEK_CUR);
        clear_route(&route);
        limpa_string(dist,10);
        fread(route.estadoOrigem,2*sizeof(char),1,csv_file);
        fgetc(csv_file);
        fread(route.estadoDestino,2*sizeof(char),1,csv_file);
        fgetc(csv_file);
        for(int i = 0; a!= ',';i++){
            dist[i]= a;
            a = fgetc(csv_file);
        }
        route.distancia = atoi(dist);
        a = fgetc(csv_file);
        for(int i = 0;a != ',';i++){
            route.cidadeOrigem[i] = a;
            a = fgetc(csv_file);
        }
        a = fgetc(csv_file);
        for(int i = 0;a != ',';i++){
            route.cidadeDestino[i] = a;
            a = fgetc(csv_file);
        }

        a = fgetc(csv_file);
        for(int i = 0; a != '\n';i++){
            route.tempoViagem[i] = a;
            a = fgetc(csv_file);
        }
        write_register(bin_file,route,1);
        head.numero_arestas++;
        if(query_city_file(head_file,route.cidadeOrigem))
            head.numero_vertices++;
        if(query_city_file(head_file,route.cidadeDestino))
            head.numero_vertices++;
    }
    head.status = '1';
    write_header(head,bin_file);
    fclose(bin_file);
    fclose(csv_file);
    fclose(head_file);
    return 0;
}
void read_variable_string(FILE *file_bin,char *string_var){
    int i = 0;
    while(1){
        string_var[i] = fgetc(file_bin);
        if(string_var[i] == '|')
            break;
        i++;
    }
    string_var[i] = '\0';
}

int read_bin_rnn(FILE* file, int rnn, Route *route){
    clear_route(route);
    fseek(file,19,SEEK_SET);
    fseek(file,rnn*TAM_REGISTRO,SEEK_CUR);
    if(fgetc(file) == EOF)
        return -1;
    fseek(file,-1,SEEK_CUR);
    fread(route->estadoOrigem,2*sizeof(char),1,file);
    //printf("%s\n",route->estadoOrigem);
    fread(route->estadoDestino,2*sizeof(char),1,file);
    //printf("%s\n",route->estadoDestino);
    fread(&(route->distancia),sizeof(int),1,file);
    //printf("%d\n",route-> distancia);
    read_variable_string(file,route->cidadeOrigem);
    //printf("aaaaa");
    read_variable_string(file,route->cidadeDestino);
    read_variable_string(file,route->tempoViagem);
    return 1;
}

int recover_data(FILE* file){
    if(file == NULL){
        printf("Falha no processamento do arquivo.");
        return -1;
    }

    if(fgetc(file) == '0'){
        printf("Falha no processamento do arquivo.");
        return -1;
    }
    int i = 0;
    int cont = 0;
    Route route;
    clear_route(&route);
    while(read_bin_rnn(file,i,&route) != -1){
        if(route.estadoOrigem[0] != '*'){
            printf("%d %s %s %d %s %s %s \n",i,route.estadoOrigem,route.estadoDestino,route.distancia,route.cidadeOrigem,
                   route.cidadeDestino,route.tempoViagem);
            cont++;
        }
        i++;
    }
    if(cont == 0)
        printf("Registro inexistente.");
    close_file(file);
    return 0;
}
int dictionary_field(char *field_name){
    char dictionary[6][15] = {"estadoOrigem","estadoDestino","distancia","cidadeOrigem","cidadeDestino","tempoViagem"};
    for(int i = 0; i <6;i++){
        if(!strcmp(field_name,dictionary[i]))
            return i+1;
    }
    return -1;
}
void recover_search(FILE* file){
    char nome_campo[15]; // nome do tipo do campo do que se busca
    char wntd_data[40]; // conteudo a se buscar
    int tipo_campo; // indica o tipo de campo lido em nome_campo
    Route route;
    clear_route(&route);
    int i=0;
    int flag=0;
    int aux;

    scanf("%s", nome_campo); // TALVEZ, pode ser os argv
    scan_quote_string(wntd_data);
    tipo_campo = dictionary_field(nome_campo);
    while (read_bin_rnn(file,i,&route) != -1)
    {
        switch (tipo_campo)
        {
            case  1 :
                if(!strcmp(wntd_data,route.estadoOrigem)){
                    printf("%d %s %s %d %s %s %s \n",i,route.estadoOrigem,route.estadoDestino,route.distancia,route.cidadeOrigem,
                           route.cidadeDestino,route.tempoViagem);
                    flag++;
                }
                break;

            case 2 :
                if (!strcmp(wntd_data,route.estadoDestino)){
                    printf("%d %s %s %d %s %s %s \n",i,route.estadoOrigem,route.estadoDestino,route.distancia,route.cidadeOrigem,
                           route.cidadeDestino,route.tempoViagem);
                    flag++;
                }
                break;

            case 3 :
                aux=atoi(wntd_data);
                if(aux==route.distancia){
                    printf("%d %s %s %d %s %s %s \n",i,route.estadoOrigem,route.estadoDestino,route.distancia,route.cidadeOrigem,
                           route.cidadeDestino,route.tempoViagem);
                    flag++;
                }
                break;

            case 4 :
                if(!strcmp(wntd_data,route.cidadeOrigem)){
                    printf("%d %s %s %d %s %s %s \n",i,route.estadoOrigem,route.estadoDestino,route.distancia,route.cidadeOrigem,
                           route.cidadeDestino,route.tempoViagem);
                    flag++;
                }
                break;

            case 5 :
                if(!strcmp(wntd_data,route.cidadeDestino)){
                    printf("%d %s %s %d %s %s %s \n",i,route.estadoOrigem,route.estadoDestino,route.distancia,route.cidadeOrigem,
                           route.cidadeDestino,route.tempoViagem);
                    flag++;
                }
                break;

            case 6 :
                if(!strcmp(wntd_data,route.tempoViagem)){
                    printf("%d %s %s %d %s %s %s \n",i,route.estadoOrigem,route.estadoDestino,route.distancia,route.cidadeOrigem,
                           route.cidadeDestino,route.tempoViagem);
                    flag++;
                }
                break;

            default:
                printf("Erro na sintaxe do tipo de campo!");
                break;
        }
        i++;
    }
    if(!flag)
        printf("Registro inexistente.");
    fclose(file);
}

void recover_rrn(FILE* file){
    Route route;
    clear_route(&route);
    int rrn;
    scanf("%d", &rrn);
    if(read_bin_rnn(file,rrn,&route)!=-1){
        printf("%d %s %s %d %s %s %s \n",rrn,route.estadoOrigem,route.estadoDestino,route.distancia,route.cidadeOrigem,
               route.cidadeDestino,route.tempoViagem);
    }else{
        printf("Registro inexistente.");
    }
    fclose(file);
}
void read_header(FILE *file, Header *head){
    fseek(file,0,SEEK_SET);
    fread(&(head->status),sizeof(char),1,file);
    fread(&(head->numero_vertices),sizeof(int),1,file);
    fread(&(head->numero_arestas),sizeof(int),1,file);
    fread(head->data_ultima_compactacao,10*sizeof(char),1,file);
}
int compact_file(char *file_name, char* compacted_file_name){
    FILE * file = open_file(file_name,"rb");
    FILE * compacted_file = open_file(compacted_file_name,"w");
    int rrn = 0;
    Route route;
    Header head;
    clear_route(&route);
    read_header(file,&head);
    get_current_date(head.data_ultima_compactacao);
    write_header(head,compacted_file);
    while(read_bin_rnn(file,rrn,&route) != -1){
        if(route.estadoOrigem[0] != '*'){
            write_register(compacted_file,route,1);
        }
        rrn++;
    }
    fclose(file);
    fclose(compacted_file);
    return 0;
}
int remove_rrn(FILE *file, int rrn){
    fseek(file,19,SEEK_SET);
    fseek(file,rrn*TAM_REGISTRO,SEEK_CUR);
    if(fgetc(file) == EOF)
        return -1;
    fseek(file,-1,SEEK_CUR);
    fwrite("*",sizeof(char),1,file);
    return 0;
}
int remove_register(char *file_name){
    FILE* file = open_file(file_name,"rb+");
    if(file == NULL){
        printf("Falha no processamento do arquivo.");
        return -1;
    }
    int num_reg;
    int distance;
    scanf("%d",&num_reg);
    for(int i =0; i< num_reg; i++){
        //fflush(stdin);
        int j = 0;
        int flag = 0;
        char field_name[15];
        char field_value[40];
        Route route;
        clear_route(&route);
        scanf("%s",field_name);
        scan_quote_string(field_value);
        int field_type = dictionary_field(field_name);
        if(field_type == 3)
            distance = atoi(field_value);
        while(read_bin_rnn(file,j,&route) != -1){
            switch(field_type){
                case 1:

                    if(!strcmp(field_value,route.estadoOrigem)){
                        remove_rrn(file,j);
                        flag++;
                    }
                    break;
                case 2:
                    if(!strcmp(field_value,route.estadoDestino)){
                        remove_rrn(file,j);
                        flag++;
                    }
                    break;
                case 3:
                    if(distance == route.distancia){
                        remove_rrn(file,j);
                        flag++;
                    }
                    break;
                case 4:
                    if(!strcmp(field_value,route.cidadeOrigem)){
                        remove_rrn(file,j);
                        flag++;
                    }
                    break;
                case 5:
                    if(!strcmp(field_value,route.cidadeDestino)){
                        remove_rrn(file,j);
                        flag++;
                    }
                    break;
                case 6:
                    if(!strcmp(field_value,route.tempoViagem)){
                        remove_rrn(file,j);
                        flag++;
                    }
                    break;
            }
            j++;
        }
        if(!flag) {
            printf("Registro inexistente.");
            return -1;
        }
    }
    return 0;
}

void insert_regs(char* fileName, int n){
    int i=0;
    char aux[40];
    Route route;
    FILE* file = open_file(fileName, "rb+");

    fseek(file,19,SEEK_SET);

    while(getc(file) != EOF){
        fseek(file,TAM_REGISTRO-1,SEEK_CUR);
        i++;
    }

    for(i=0; i<n; i++){
        clear_route(&route);
        scan_quote_string(aux);
        if(!strcmp(aux, "NULO")){
            limpa_string(route.estadoOrigem,3);
        }else{
            strcpy(route.estadoOrigem, aux);
        }
        scan_quote_string(aux);
        if(!strcmp(aux, "NULO")){
            limpa_string(route.estadoDestino,3);
        }else{
            strcpy(route.estadoDestino, aux);
        }
        scan_quote_string(aux);
        if(!strcmp(aux, "NULO")){
            route.distancia=0;
        }else{
            route.distancia = atoi(aux);
        }
        scan_quote_string(aux);
        if(!strcmp(aux, "NULO")){
            limpa_string(route.cidadeOrigem,40);
        }else{
            strcpy(route.cidadeOrigem, aux);
        }
        scan_quote_string(aux);
        if(!strcmp(aux, "NULO")){
            limpa_string(route.cidadeDestino,40);
        }else{
            strcpy(route.cidadeDestino, aux);
        }
        scan_quote_string(aux);
        if(!strcmp(aux, "NULO")){
            limpa_string(route.tempoViagem,10);
        }else{
            strcpy(route.tempoViagem, aux);
        }
        write_register(file, route,1);
    }
    close_file(file);
}

int atualiza_campo_rrn(char *fileName, int n){
    FILE *file = open_file(fileName, "rb+");
    int i, tipo, read;
    int rrn;
    char tipoCampo[15];
    char novoCampo[40];
    Route reg;
    clear_route(&reg);

    for(i=0; i<n; i++){
        scanf ("%d %s", &rrn, tipoCampo);
        scan_quote_string(novoCampo);
        fseek(file,19,SEEK_SET);
        fseek(file,rrn*TAM_REGISTRO,SEEK_CUR);

        tipo = dictionary_field(tipoCampo);
        if(read_bin_rnn(file, rrn, &reg) == -1){
            printf("Falha no processamento do arquivo.");
            return -1;
        }
        switch (tipo)
        {
            case 1:
                if(!strcmp(novoCampo, "NULO")){
                    strcpy(reg.estadoOrigem, novoCampo);
                }else{
                    limpa_string(reg.estadoOrigem,3);
                }
                break;

            case 2:
                if(!strcmp(novoCampo, "NULO")){
                    strcpy(reg.estadoDestino, novoCampo);
                }else{
                    limpa_string(reg.estadoDestino,3);
                }
                break;

            case 3:
                if(!strcmp(novoCampo, "NULO")){
                    reg.distancia=atoi(novoCampo);
                }else{
                    reg.distancia=0;
                }
                break;

            case 4:
                if (!strcmp(novoCampo, "NULO")){
                    strcpy(reg.cidadeOrigem, novoCampo);
                }else{
                    limpa_string(reg.cidadeOrigem,40);
                }
                break;

            case 5:
                if (!strcmp(novoCampo, "NULO")){
                    strcpy(reg.cidadeDestino, novoCampo);
                }else{
                    limpa_string(reg.cidadeDestino,40);
                }
                break;

            case 6:
                if (!strcmp(novoCampo, "NULO")){
                    strcpy(reg.tempoViagem, novoCampo);
                }else{
                    limpa_string(reg.tempoViagem, 10);
                }
                break;

            default:
                printf("Falha no processamento do arquivo.");
                break;
        }
        fseek(file,19,SEEK_SET);
        fseek(file,rrn*TAM_REGISTRO,SEEK_CUR);
        write_register(file, reg, 0);
    }
    close_file(file);
}

int main(){
    char funcao;
    char fileNameCSV[20];
    char fileNameBin[20];
    char compacted_file_name[40];
    int n;

    scanf("%c", &funcao);

    switch(funcao){
        case '1':		// LEITURA DE DADOS
            scanf("%s %s", fileNameCSV,fileNameBin);
            if(read_csv(fileNameCSV,fileNameBin) != -1)
                binarioNaTela1(fileNameBin);

            break;
        case '2':		// RECUPERAÇÃO DE TODOS OS REGISTROS
            scanf("%s", fileNameBin);
            recover_data(open_file(fileNameBin,"rb"));

            break;
        case '3':		// RECUPERAÇÃO POR BUSCA
            scanf("%s", fileNameBin);
            recover_search(open_file(fileNameBin,"rb"));

            break;
        case '4':		// RECUPERAÇÃO DE REGISTROS POR RRN
            scanf("%s", fileNameBin);
            recover_rrn(open_file(fileNameBin,"rb"));

            break;
        case '5':		// REMOÇÃO DE REGISTROS
            scanf("%s", fileNameBin);
            if(remove_register(fileNameBin) != -1)
                binarioNaTela1(fileNameBin);

            break;
        case '6':		// INSERÇÃO DE REGISTROS ADICIONAIS
            scanf("%s %d", fileNameBin, &n);
            //binarioNaTela1(fileNameBin);
            insert_regs(fileNameBin, n);
            binarioNaTela1(fileNameBin);
            break;

        case '7':		// ATUALIZAÇÃO DE REGISTRO POR RRN
            scanf("%s %d", fileNameBin, &n);
            atualiza_campo_rrn(fileNameBin, n);
            binarioNaTela1(fileNameBin);
            break;

        case '8':		// COMPACTAÇÃO DO ARQUIVO
            scanf("%s", compacted_file_name);
            compact_file(fileNameBin,compacted_file_name);
            break;


    }
    return 0;
}