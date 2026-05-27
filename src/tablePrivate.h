/**
 * @file tablePrivate.h
 */


#pragma once

//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdint.h>
#include <stdbool.h>

#include "table.h"

//***************************************************************************//
//**************************  PUBLIC DEFINES ********************************//
//***************************************************************************//

#define V_ALIGN_IDX  0
#define H_ALIGN_IDX  1
#define BORDER_L_IDX 2
#define BORDER_T_IDX 3
#define BIT_V_ALIGN  (1 << V_ALIGN_IDX)
#define BIT_H_ALIGN  (1 << H_ALIGN_IDX)
#define BIT_BORDER_L (1 << BORDER_L_IDX)
#define BIT_BORDER_T (1 << BORDER_T_IDX)

//***************************************************************************//
//************************** PUBLIC TYPEDEFS ********************************//
//***************************************************************************//

struct TableStyle
{
    TableVerticalAlignment   vAlign;
    TableHorizontalAlignment hAlign;
    TableBorderStyle         borderLeft;
    TableBorderStyle         borderTop;
    uint16_t                 overrideFlags;
};

typedef struct
{
    uint16_t   x;
    uint16_t   y;
    TableStyle style;
    char*      content; // Heap

    uint16_t parentX;
    uint16_t parentY;
    uint16_t colSpan;
    uint16_t rowSpan;
    bool     hasParent;
} Cell;

typedef struct
{
    uint16_t   colIdx;
    TableStyle style;
    uint16_t   minWidth;
} Column;

typedef struct
{
    uint16_t   rowIdx;
    TableStyle style;
    Cell*      cells; // Heap
    uint16_t   minHeight;
} Row;

struct Table
{
    uint16_t   numRows;
    uint16_t   numCols;
    Column*    columns;
    Row**      rows;
    TableStyle defaultStyle;
    uint16_t (*stringWidthFunction)(const char*);
    uint16_t cursorRow;
    uint16_t cursorCol;
};
