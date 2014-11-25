/*
 * \file spreadsheet.c
 * Implementação do arquivo spreadsheet.h
 */

#include "spreadsheet.h"

// Largura total da janela
#define WINDOW_WIDTH 130
// Altura total da janela
#define WINDOW_HEIGHT 40

// Posicionamento da matriz de células
#define WINDOW_CELLS_X 0
#define WINDOW_CELLS_Y 0

// Posicionamento e dimensões da tela de instruções
#define WINDOW_INSTRUCTION_X 10
#define WINDOW_INSTRUCTION_Y 24
#define WINDOW_INSTRUCTION_WIDTH 110
#define WINDOW_INSTRUCTION_HEIGHT 8

// Posicionamento e dimensões da tela de usuário
#define WINDOW_USER_X WINDOW_INSTRUCTION_X
#define WINDOW_USER_Y WINDOW_INSTRUCTION_Y+WINDOW_INSTRUCTION_HEIGHT
#define WINDOW_USER_WIDTH WINDOW_INSTRUCTION_WIDTH
#define WINDOW_USER_HEIGHT WINDOW_INSTRUCTION_HEIGHT

// Posicionamento e dimensões da tela de opções
#define WINDOW_SELECT_X WINDOW_INSTRUCTION_X+WINDOW_INSTRUCTION_WIDTH
#define WINDOW_SELECT_Y WINDOW_INSTRUCTION_Y
#define WINDOW_SELECT_WIDTH WINDOW_WIDTH-WINDOW_INSTRUCTION_WIDTH
#define WINDOW_SELECT_HEIGHT 16

// Posicionamento e dimensões da tela de listagem
#define WINDOW_LIST_X 10
#define WINDOW_LIST_Y 0
#define WINDOW_LIST_WIDTH 50
#define WINDOW_LIST_HEIGHT 24

// opções do loop principal da função run()
#define OPTION_SELECT_CELL "Selecionar celula"
#define OPTION_INSERT_EXPRESSION "Inserir expressao"
#define OPTION_UNDO "Desfazer ultima operacao"
#define OPTION_REDO "Refazer ultima operacao"
#define OPTION_SAVE "Salvar espaco de trabalho"
#define OPTION_EXIT "Sair"

// opções YES/NO
#define YES "Sim"
#define NO "Nao"

// dimensões de linha e coluna
#define ROW 1
#define COLUMN 1

/*******************************************************************************
 * Funções privadas
 *******************************************************************************/

/**
 * Atualiza gráfico de células
 * \param matrix Ponteiro para informações da matriz de célula
 * \param graphic Ponteiro para gráfico de células
 */
void SPREADSHEET_updateGraphicCells(Matrix** matrix, GraphicCells** graphic){

    if(!matrix || !(*matrix) || !graphic || !(*graphic)) return;

    // guarda linha e coluna
    int row, column;
    // guarda valor da célula
    int value;
    // guarda expressão da célula
    char expression[70];

    for(row=1 ; row<=ROWS ; row++){
        for(column=1 ; column<=COLUMNS ; column++){
            // pega expressão da célula
            MATRIX_getExpression(&(*matrix), row, column, expression);
            // se expressão não vazia, atualiza gráfico
            if(strcmp(expression,"")!=0){
                value = MATRIX_getValue(&(*matrix), row, column);
                GRAPHICSCELLS_updateCell(&(*graphic), row, column, value, KEEP_MARK, false);
            }
        }
    }

}

/*******************************************************************************
 * Funções públicas
 *******************************************************************************/

/**
 * Inicia sistema principal de planilha
 * \param matrix Ponteiro para matriz de células. Informe NULL no caso de
 * novo espaço de trabalho
 * \param workspaceName Nome do espaço de trabalho. Informe NULL no caso de
 * novo espaço de trabalho
 */
void SPREADSHEET_run(Matrix** matrix, const char* workspaceName){

    // variáveis que guardam a linha inicial e coluna inicial
    int currentRow=1, currentColumn=1;

    // Ponteiro para a janela de usuário
    GraphicUser* graphic_user = NULL;
    // Ponteiro para a janela de instruções
    GraphicInstructions* graphic_instructions = NULL;
    // Ponteiro para a janela de listagem
    GraphicInstructions* graphic_list = NULL;
    // Ponteiro para a janela de opções
    GraphicSelect* graphic_select = NULL;
    // Ponteiro para a janela de células
    GraphicCells* graphic_cells = NULL;

    // Ponteiro para dados de salvamento
    SaveFile* save = SAVE_create(SAVEFILE);

    // Cria janelas de usuário, de instruções e de opções
    graphic_user = GRAPHICUSER_create(WINDOW_USER_X,WINDOW_USER_Y, WINDOW_USER_WIDTH,
            WINDOW_USER_HEIGHT);
    graphic_instructions = GRAPHICINST_create(WINDOW_INSTRUCTION_X, WINDOW_INSTRUCTION_Y,
            WINDOW_INSTRUCTION_WIDTH, WINDOW_INSTRUCTION_HEIGHT);
    graphic_select = GRAPHICSSELECT_create(WINDOW_SELECT_X,WINDOW_SELECT_Y,
            WINDOW_SELECT_WIDTH, WINDOW_SELECT_HEIGHT);

    // se nome de espaço de trabalho não definido, define
    if(!workspaceName){

        // cria janela de listagem
        graphic_list = GRAPHICINST_create(WINDOW_LIST_X,WINDOW_LIST_Y,
                WINDOW_LIST_WIDTH, WINDOW_LIST_HEIGHT);

        // define espaço de trabalho
        while(SAVE_workspaceIsNULL(&save))
            SAVE_defineWorkspace(&save, &graphic_instructions, &graphic_list,
                    &graphic_user, &graphic_select, NULL);

        // desaloca janela de listagem
        graphic_list = GRAPHICINST_free(graphic_list);

    }

    // nome do espaço de trabalho definido
    else
        SAVE_defineWorkspace(&save, NULL, NULL, NULL, NULL, workspaceName);

    // cria o gráfico da matriz
    graphic_cells = GRAPHICSCELLS_create(WINDOW_CELLS_X,WINDOW_CELLS_Y,ROWS,COLUMNS);

    // Inicializa matriz se não inicializada
    if(!matrix){
        // cria a matriz
        Matrix* newMatrix = MATRIX_create(ROWS, COLUMNS);
        matrix = &newMatrix;
        *matrix = newMatrix;
    }
    // caso já tenha sido inicializada
    else{
        // neste caso devemos percorrer a matriz e atualizar o gráfico das células
        SPREADSHEET_updateGraphicCells(&(*matrix),&graphic_cells);
    }

    // Ponteiro para undo_redo_cells
    UndoRedoCells* undoRedo = UNDOREDOCELLS_create();

    // guarda opção escolhida
    char option[30];

    // controla loop principal
    int mainLoop = true;

    // loop principal
    while(mainLoop){

        // prepara janela de seleção
        GRAPHICSSELECT_clearOptions(&graphic_select);

        // adiciona opções
        GRAPHICSSELECT_addOption(&graphic_select, OPTION_SELECT_CELL);
        GRAPHICSSELECT_addOption(&graphic_select, OPTION_INSERT_EXPRESSION);
        if(UNDOREDOCELLS_canUndo(&undoRedo))
            GRAPHICSSELECT_addOption(&graphic_select, OPTION_UNDO);
        if(UNDOREDOCELLS_canRedo(&undoRedo))
            GRAPHICSSELECT_addOption(&graphic_select, OPTION_REDO);
        GRAPHICSSELECT_addOption(&graphic_select, OPTION_SAVE);
        GRAPHICSSELECT_addOption(&graphic_select, OPTION_EXIT);

        // pede para o usuário escolher
        GRAPHICINST_clear(&graphic_instructions);
        GRAPHICINST_write(&graphic_instructions, "Escolha uma opcao:",
                COLUMN*1, ROW*1);
        GRAPHICINST_writeKeyboard(&graphic_instructions,COLUMN*1, ROW*2, false);

        // abre a janela de opções e guarda resultado da escolha em option
        GRAPHICSSELECT_selectOption(&graphic_select, option);

        // verifica opção escolhida

        // se for selecionar célula
        if(strcmp(option,OPTION_SELECT_CELL)==0){

        }

        // se for inserir expressão
        else if(strcmp(option, OPTION_INSERT_EXPRESSION)==0){

        }

        // se for desfazer
        else if(strcmp(option, OPTION_UNDO)==0){

        }

        // se for refazer
        else if(strcmp(option, OPTION_REDO)==0){

        }

        // se for salvar espaço de trabalho
        else if(strcmp(option, OPTION_SAVE)==0){

        }

        // se for sair
        else{

        }
    }

    // limpa telas
    GRAPHICINST_clear(&graphic_instructions);
    GRAPHICSSELECT_clearOptions(&graphic_select);
    GRAPHICUSER_clear(&graphic_user);

    // libera memória da matriz de célula
    (*matrix) = MATRIX_free((*matrix));

    // libera memória do save
    save = SAVE_free(save);

    // libera memória do undoRedo
    undoRedo = UNDOREDOCELLS_free(undoRedo);

    // libera memória de todos os objetos gráficos
    graphic_instructions = GRAPHICINST_free(graphic_instructions);
    graphic_cells = GRAPHICSCELLS_free(graphic_cells);
    graphic_select = GRAPHICSSELECT_free(graphic_select);
    graphic_user = GRAPHICUSER_free(graphic_user);

}










