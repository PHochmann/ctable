/**
 * @file table.c
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "table.h"
#include "tableRenderer.h"
#include "tablePrivate.h"

//***************************************************************************//
//**************************  PRIVATE DEFINES *******************************//
//***************************************************************************//


//***************************************************************************//
//************************** PRIVATE TYPEDEFS *******************************//
//***************************************************************************//


//***************************************************************************//
//************************** PRIVATE VARIABLE DECLARATIONS ******************//
//***************************************************************************//


//***************************************************************************//
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//

static bool doesColExist(const Table* table, uint16_t colIdx);

static bool doesRowExist(const Table* table, uint16_t rowIdx);

static bool doesCellExist(const Table* table, uint16_t rowIdx, uint16_t colIdx);

static Cell* getCell(const Table* table, uint16_t rowIdx, uint16_t colIdx);

static Cell* getCellOwner(const Table* table, uint16_t rowIdx, uint16_t colIdx);

static bool ensureRowExists(Table* table, uint16_t rowIdx);

static bool ensureCellExists(Table* table, uint16_t rowIdx, uint16_t colIdx);

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

static bool doesColExist(const Table* table, uint16_t colIdx)
{
    return colIdx < table->numCols;
}

static bool doesRowExist(const Table* table, uint16_t rowIdx)
{
    return (rowIdx < table->numRows) && (table->rows[rowIdx] != NULL);
}

static bool doesCellExist(const Table* table, uint16_t rowIdx, uint16_t colIdx)
{
    return doesColExist(table, colIdx) && doesRowExist(table, rowIdx);
}

static Cell* getCell(const Table* table, uint16_t rowIdx, uint16_t colIdx)
{
    if(doesCellExist(table, rowIdx, colIdx) == false)
    {
        return NULL; // Out of bounds
    }
    return &table->rows[rowIdx]->cells[colIdx];
}

static Cell* getCellOwner(const Table* table, uint16_t rowIdx, uint16_t colIdx)
{
    Cell* cell = getCell(table, rowIdx, colIdx);
    if(cell == NULL)
    {
        return NULL;
    }

    if(cell->hasParent == false)
    {
        return cell;
    }

    return getCell(table, cell->parentY, cell->parentX);
}

static bool ensureRowExists(Table* table, uint16_t rowIdx)
{
    if(doesRowExist(table, rowIdx))
    {
        return true; // Row already exists
    }

    // Grow row array if necessary
    if(rowIdx >= table->numRows)
    {
        table->rows    = realloc(table->rows, (rowIdx + 1) * sizeof(Row*));
        table->numRows = rowIdx + 1;
        // Set inbetween rows to NULL
        for(uint16_t r = table->numRows; r < rowIdx; r++)
        {
            table->rows[r] = NULL;
        }
    }

    // Set new row in row array
    table->rows[rowIdx]  = malloc(sizeof(Row));
    *table->rows[rowIdx] = (Row){
        .rowIdx = rowIdx,
        .style  = { 0 },                               // No styles set by default, so overrideFlags is 0
        .cells  = calloc(table->numCols, sizeof(Cell)) // Allocate cells for the new row
    };

    for(uint16_t c = 0; c < table->numCols; c++)
    {
        table->rows[rowIdx]->cells[c] = (Cell){
            .x         = c,
            .y         = rowIdx,
            .style     = { 0 }, // No styles set by default, so overrideFlags is 0
            .content   = NULL,  // No content by default
            .hasParent = false,
            .parentX   = 0,
            .parentY   = 0,
            .colSpan   = 1,
            .rowSpan   = 1
        };
    }

    return true;
}

static bool ensureCellExists(Table* table, uint16_t rowIdx, uint16_t colIdx)
{
    if(doesColExist(table, colIdx) == false)
    {
        return false; // Column out of bounds
    }
    if(ensureRowExists(table, rowIdx) == false)
    {
        return false; // Failed to ensure row exists
    }
    return true;
}

//***************************************************************************//
//************************** PUBLIC FUNCTION DEFINITIONS ********************//
//***************************************************************************//

Table* table_create(uint16_t numColumns)
{
    Table* result = malloc(sizeof(Table));
    if(result == NULL)
    {
        return NULL;
    }

    *result = (Table){
        .defaultStyle = {
            .vAlign        = TABLE_V_ALIGN_TOP,
            .hAlign        = TABLE_H_ALIGN_LEFT,
            .borderLeft    = TABLE_BORDER_NONE,
            .borderTop     = TABLE_BORDER_NONE,
            .overrideFlags = 0x0F // All styles need to be overriden since defaultStyle is the fallback
        },
        .columns             = calloc(numColumns, sizeof(Column)),
        .numRows             = 0,
        .numCols             = numColumns,
        .rows                = NULL,
        .stringWidthFunction = NULL,
        .cursorRow           = 0,
        .cursorCol           = 0
    };

    return result;
}

void table_destroy(Table* table)
{
    if(table == NULL)
    {
        return;
    }

    for(uint16_t r = 0; r < table->numRows; r++)
    {
        if(table->rows[r] != NULL)
        {
            for(uint16_t c = 0; c < table->numCols; c++)
            {
                free(table->rows[r]->cells[c].content);
            }
            free(table->rows[r]->cells);
            free(table->rows[r]);
        }
    }
    free(table->rows);
    free(table->columns);
    free(table);
}

void table_print(const Table* table)
{
    tableRenderer_render(table, stdout);
}

void table_fprint(const Table* table, FILE* stream)
{
    tableRenderer_render(table, stream);
}

TableStyle* table_getDefaultStyle(Table* table)
{
    return &table->defaultStyle;
}

TableStyle* table_getRowStyle(Table* table, uint16_t rowIdx)
{
    if(ensureRowExists(table, rowIdx) == false)
    {
        return NULL;
    }
    return &table->rows[rowIdx]->style;
}

TableStyle* table_getColStyle(Table* table, uint16_t colIdx)
{
    if(doesColExist(table, colIdx) == false)
    {
        return NULL;
    }
    return &table->columns[colIdx].style;
}

TableStyle* table_getCellStyle(Table* table, uint16_t rowIdx, uint16_t colIdx)
{
    if(ensureCellExists(table, rowIdx, colIdx) == false)
    {
        return NULL;
    }
    return &table->rows[rowIdx]->cells[colIdx].style;
}

void table_setVAlign(TableStyle* styleRef, TableVerticalAlignment vAlign)
{
    if(styleRef == NULL)
    {
        return;
    }
    styleRef->vAlign = vAlign;
    styleRef->overrideFlags |= BIT_V_ALIGN;
}

void table_setHAlign(TableStyle* styleRef, TableHorizontalAlignment hAlign)
{
    if(styleRef == NULL)
    {
        return;
    }
    styleRef->hAlign = hAlign;
    styleRef->overrideFlags |= BIT_H_ALIGN;
}

void table_setBorderLeft(TableStyle* styleRef, TableBorderStyle borderLeft)
{
    if(styleRef == NULL)
    {
        return;
    }
    styleRef->borderLeft = borderLeft;
    styleRef->overrideFlags |= BIT_BORDER_L;
}

void table_setBorderTop(TableStyle* styleRef, TableBorderStyle borderTop)
{
    if(styleRef == NULL)
    {
        return;
    }
    styleRef->borderTop = borderTop;
    styleRef->overrideFlags |= BIT_BORDER_T;
}

bool table_setCellText(Table* table, uint16_t rowIdx, uint16_t colIdx, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool result = table_vsetCellText(table, rowIdx, colIdx, fmt, args);
    va_end(args);
    return result;
}

bool table_vsetCellText(Table* table, uint16_t rowIdx, uint16_t colIdx, const char* fmt, va_list args)
{
    if(ensureCellExists(table, rowIdx, colIdx) == false)
    {
        return false;
    }
    int result = vasprintf(&getCell(table, rowIdx, colIdx)->content, fmt, args);
    return result != -1;
}

void table_setMinColWidth(Table* table, uint16_t colIdx, uint16_t width)
{
    if(doesColExist(table, colIdx) == false)
    {
        return;
    }
    table->columns[colIdx].minWidth = width;
}

void table_setMinRowHeight(Table* table, uint16_t rowIdx, uint16_t height)
{
    if(ensureRowExists(table, rowIdx) == false)
    {
        return;
    }
    table->rows[rowIdx]->minHeight = height;
}

bool table_setCellSpan(Table* table, uint16_t rowIdx, uint16_t colIdx, uint16_t colSpan, uint16_t rowSpan)
{
    if(doesColExist(table, colIdx + colSpan - 1) == false)
    {
        return false;
    }
    for(uint16_t r = rowIdx; r < rowIdx + rowSpan; r++)
    {
        if(ensureRowExists(table, r) == false)
        {
            return false;
        }
        for(uint16_t c = colIdx; c < colIdx + colSpan; c++)
        {
            if((r != rowIdx) || (c != colIdx)) // Skip top-left cell
            {
                Cell* cell      = getCell(table, r, c);
                cell->hasParent = true;
                cell->parentX   = colIdx;
                cell->parentY   = rowIdx;
            }
        }
    }
    Cell* topLeftCell    = getCell(table, rowIdx, colIdx);
    topLeftCell->colSpan = colSpan;
    topLeftCell->rowSpan = rowSpan;
    return true;
}

void table_setStringWidthFunction(Table* table, uint16_t (*stringWidthFunction)(const char*))
{
    table->stringWidthFunction = stringWidthFunction;
}

bool table_setCursor(Table* table, uint16_t rowIdx, uint16_t colIdx)
{
    table->cursorRow = rowIdx;
    table->cursorCol = colIdx;
    return true;
}

bool table_setCurrentCellText(Table* table, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool result = table_vsetCellText(table, table->cursorRow, table->cursorCol, fmt, args);
    va_end(args);
    table->cursorCol++;
    return result;
}

void table_nextRow(Table* table)
{
    table->cursorRow++;
    table->cursorCol = 0;
}

TableStyle* table_getCurrentCellStyle(Table* table)
{
    return table_getCellStyle(table, table->cursorRow, table->cursorCol);
}

TableStyle* table_getCurrentRowStyle(Table* table)
{
    return table_getRowStyle(table, table->cursorRow);
}

TableStyle* table_getCurrentColStyle(Table* table)
{
    return table_getColStyle(table, table->cursorCol);
}
