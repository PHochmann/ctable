/**
 * @file tableRenderer.c
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "tableRenderer.h"

//***************************************************************************//
//**************************  PRIVATE DEFINES *******************************//
//***************************************************************************//

#define MAX(a, b) ((a) > (b) ? (a) : (b))

//***************************************************************************//
//************************** PRIVATE TYPEDEFS *******************************//
//***************************************************************************//

typedef struct
{
    uint16_t currPos;
    uint16_t lines;
    char*    content;
} LineReader;

typedef struct
{
    uint16_t height;
    bool     hasTopBorder;
} RenderedRow;

typedef struct
{
    uint16_t width;
    bool     hasLeftBorder;
} RenderedColumn;

typedef struct
{
    uint16_t   contentWidth;
    uint16_t   contentHeight;
    uint16_t   currLine;
    LineReader contentReader;
} RenderedCell;

typedef struct
{
    const Table*    table;
    RenderedRow*    renderedRows;
    RenderedColumn* renderedCols;
    RenderedCell*   renderedCells;
} RendererContext;

//***************************************************************************//
//************************** PRIVATE VARIABLE DECLARATIONS ******************//
//***************************************************************************//

static const char* borders[][12] = {
    [TABLE_BORDER_NONE]   = { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " },
    [TABLE_BORDER_SOLID]  = { "┌", "┬", "┐", "├", "┼", "┤", "└", "┴", "┘", "─", "│", " " },
    [TABLE_BORDER_DOUBLE] = { "╔", "╦", "╗", "╠", "╬", "╣", "╚", "╩", "╝", "═", "║", " " },
    [TABLE_BORDER_DASHED] = { "+", "+", "+", "+", "+", "+", "+", "+", "+", "-", "|", " " }
};

static const size_t borderLookup[16] = { 11, 11, 11, 6, 11, 10, 0, 3, 11, 8, 9, 7, 2, 5, 1, 4 };

//***************************************************************************//
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//

static const TableStyle* getEffectiveStyleWithRespectTo(const TableStyle* globalStyle,
    const TableStyle*                                                     colStyle,
    const TableStyle*                                                     rowStyle,
    const TableStyle*                                                     cellStyle,
    uint8_t                                                               index);

static TableStyle getEffectiveStyle(const Table* table, uint16_t rowIdx, uint16_t colIdx);

static void calculateTextSize(const char* text, uint16_t (*getStringWidth)(const char*), uint16_t* outWidth, uint16_t* outHeight);

static bool applySpanConstraint(uint16_t* sizes, uint16_t startIdx, uint16_t span, uint16_t requiredSize);

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

static Cell* getCell(const Table* table, uint16_t rowIdx, uint16_t colIdx)
{
    return &table->rows[rowIdx]->cells[colIdx];
}

static LineReader createLineReader(const char* text)
{
    LineReader reader = {
        .lines   = 0,
        .content = strdup(text)
    };

    // Change all \n to \0
    for(char* p = reader.content; *p != '\0'; p++)
    {
        if(*p == '\n')
        {
            *p = '\0';
            reader.lines++;
        }
    }
    reader.lines++; // Count the last line
    return reader;
}

static const char* getLine(const LineReader* reader, uint16_t line)
{
    if(line >= reader->lines)
    {
        return NULL;
    }

    const char* p = reader->content;
    for(uint16_t i = 0; i < line; i++)
    {
        p += strlen(p) + 1;
    }
    return p;
}

static void destroyLineReader(LineReader* reader)
{
    free(reader->content);
}

static const TableStyle* getEffectiveStyleWithRespectTo(const TableStyle* globalStyle,
    const TableStyle*                                                     colStyle,
    const TableStyle*                                                     rowStyle,
    const TableStyle*                                                     cellStyle,
    uint8_t                                                               index)
{
    const TableStyle* styles[4] = { cellStyle, rowStyle, colStyle, globalStyle };
    for(uint8_t i = 0; i < 4; i++)
    {
        if((styles[i] != NULL) && (styles[i]->overrideFlags & (1 << index)))
        {
            return styles[i];
        }
    }
    // Should never happen since globalStyle is always non-NULL and has all override flags set.
    return NULL;
}

static TableStyle getEffectiveStyle(const Table* table, uint16_t rowIdx, uint16_t colIdx)
{
    const TableStyle* globalStyle = &table->defaultStyle;
    const TableStyle* colStyle    = &table->columns[colIdx].style;
    const TableStyle* rowStyle    = &table->rows[rowIdx]->style;
    const TableStyle* cellStyle   = &table->rows[rowIdx]->cells[colIdx].style;

    TableStyle effectiveStyle;
    effectiveStyle.vAlign     = getEffectiveStyleWithRespectTo(globalStyle, colStyle, rowStyle, cellStyle, V_ALIGN_IDX)->vAlign;
    effectiveStyle.hAlign     = getEffectiveStyleWithRespectTo(globalStyle, colStyle, rowStyle, cellStyle, H_ALIGN_IDX)->hAlign;
    effectiveStyle.borderLeft = getEffectiveStyleWithRespectTo(globalStyle, colStyle, rowStyle, cellStyle, BORDER_L_IDX)->borderLeft;
    effectiveStyle.borderTop  = getEffectiveStyleWithRespectTo(globalStyle, colStyle, rowStyle, cellStyle, BORDER_T_IDX)->borderTop;

    return effectiveStyle;
}

static void calculateTextSize(const char* text, uint16_t (*getStringWidth)(const char*), uint16_t* outWidth, uint16_t* outHeight)
{
    uint16_t    maxWidth  = 0;
    uint16_t    lineCount = 0;
    LineReader  reader    = createLineReader(text);
    const char* currLine  = NULL;

    while((currLine = getLine(&reader, lineCount)) != NULL)
    {
        uint16_t lineWidth = getStringWidth(currLine);
        if(lineWidth > maxWidth)
        {
            maxWidth = lineWidth;
        }
        lineCount++;
    }

    destroyLineReader(&reader);
    *outWidth  = maxWidth;
    *outHeight = lineCount;
}

static RenderedCell* getRenderedCell(const RendererContext* ctx, uint16_t rowIdx, uint16_t colIdx)
{
    return &ctx->renderedCells[(rowIdx * ctx->table->numCols) + colIdx];
}

static void calculateVLines(const Table* table, bool* outVLines)
{
    for(uint16_t c = 0; c < table->numCols; c++)
    {
        outVLines[c] = false;
        for(uint16_t r = 0; r < table->numRows; r++)
        {
            Cell* cell = getCell(table, r, c);
            if((cell == NULL) || (cell->hasParent))
            {
                continue;
            }

            if(getEffectiveStyle(table, r, c).borderLeft != TABLE_BORDER_NONE)
            {
                outVLines[c] = true;
                break;
            }
        }
    }
}

static void calculateHLines(const Table* table, bool* outHLines)
{
    for(uint16_t r = 0; r < table->numRows; r++)
    {
        outHLines[r] = false;
        for(uint16_t c = 0; c < table->numCols; c++)
        {
            Cell* cell = getCell(table, r, c);
            if((cell == NULL) || (cell->hasParent))
            {
                continue;
            }

            if(getEffectiveStyle(table, r, c).borderTop != TABLE_BORDER_NONE)
            {
                outHLines[r] = true;
                break;
            }
        }
    }
}

static bool applySpanConstraint(uint16_t* sizes,
    uint16_t                              startIdx,
    uint16_t                              span,
    uint16_t                              requiredSize)
{
    uint16_t currentSize = 0;
    for(uint16_t i = 0; i < span; i++)
    {
        currentSize += sizes[startIdx + i];
    }

    if(currentSize >= requiredSize)
    {
        return false;
    }

    uint16_t deficit   = requiredSize - currentSize;
    uint16_t baseAdd   = deficit / span;
    uint16_t remainder = deficit % span;

    for(uint16_t i = 0; i < span; i++)
    {
        uint16_t add = baseAdd;
        if(i < remainder)
        {
            add++;
        }
        sizes[startIdx + i] = sizes[startIdx + i] + add;
    }

    return true;
}

static void calculateRowHeights(const RendererContext* ctx, uint16_t* outRowHeights)
{
    const Table* table = ctx->table;

    for(uint16_t r = 0; r < table->numRows; r++)
    {
        outRowHeights[r] = table->rows[r]->minHeight;
    }

    bool changed;
    do
    {
        changed = false;
        for(uint16_t r = 0; r < table->numRows; r++)
        {
            for(uint16_t c = 0; c < table->numCols; c++)
            {
                Cell* cell = getCell(table, r, c);
                if((cell == NULL) || (cell->hasParent))
                {
                    continue;
                }

                uint16_t requiredHeight = getRenderedCell(ctx, r, c)->contentHeight;
                uint16_t hLines         = 0;
                for(uint16_t i = 0; i < cell->rowSpan; i++)
                {
                    if(ctx->renderedRows[r + i].hasTopBorder)
                    {
                        hLines++;
                    }
                }

                if(applySpanConstraint(outRowHeights, r, cell->rowSpan, MAX(0, requiredHeight - hLines)) == true)
                {
                    changed = true;
                }
            }
        }
    } while(changed);
}

static void calculateColumnWidths(const RendererContext* ctx, uint16_t* outColWidths)
{
    const Table* table = ctx->table;

    for(uint16_t c = 0; c < table->numCols; c++)
    {
        outColWidths[c] = table->columns[c].minWidth;
    }

    bool changed;
    do
    {
        changed = false;
        for(uint16_t r = 0; r < table->numRows; r++)
        {
            for(uint16_t c = 0; c < table->numCols; c++)
            {
                Cell* cell = getCell(table, r, c);
                if((cell == NULL) || (cell->hasParent))
                {
                    continue;
                }

                uint16_t requiredWidth = getRenderedCell(ctx, r, c)->contentWidth;
                uint16_t vLines        = 0;
                for(uint16_t i = 0; i < cell->colSpan; i++)
                {
                    if(ctx->renderedCols[c + i].hasLeftBorder)
                    {
                        vLines++;
                    }
                }
                if(applySpanConstraint(outColWidths, c, cell->colSpan, MAX(0, requiredWidth - vLines)) == true)
                {
                    changed = true;
                }
            }
        }
    } while(changed);
}

static RendererContext createRendererContext(const Table* table)
{
    RendererContext context = (RendererContext){
        .table         = table,
        .renderedRows  = calloc(table->numRows, sizeof(RenderedRow)),
        .renderedCols  = calloc(table->numCols, sizeof(RenderedColumn)),
        .renderedCells = calloc(table->numRows * table->numCols, sizeof(RenderedCell))
    };

    // 1. Calculate cell content sizes
    for(uint16_t colIdx = 0u; colIdx < table->numCols; colIdx++)
    {
        for(uint16_t rowIdx = 0u; rowIdx < table->numRows; rowIdx++)
        {
            uint16_t width  = 0u;
            uint16_t height = 0u;
            calculateTextSize(table->rows[rowIdx]->cells[colIdx].content, table->stringWidthFunction, &width, &height);

            context.renderedCells[rowIdx * table->numCols + colIdx] = (RenderedCell){
                .contentWidth  = width,
                .contentHeight = height,
                .contentReader = createLineReader(table->rows[rowIdx]->cells[colIdx].content)
            };
        }
    }

    // 2. Calculate V and H lines
    bool* outVLines = calloc(table->numCols, sizeof(bool));
    calculateVLines(table, outVLines);
    for(uint16_t colIdx = 0u; colIdx < table->numCols; colIdx++)
    {
        context.renderedCols[colIdx].hasLeftBorder = outVLines[colIdx];
    }
    free(outVLines);

    bool* outHLines = calloc(table->numRows, sizeof(bool));
    calculateHLines(table, outHLines);
    for(uint16_t rowIdx = 0u; rowIdx < table->numRows; rowIdx++)
    {
        context.renderedRows[rowIdx].hasTopBorder = outHLines[rowIdx];
    }
    free(outHLines);

    // 3. Calculate column widths and row heights
    uint16_t* colWidths = calloc(table->numCols, sizeof(uint16_t));
    calculateColumnWidths(&context, colWidths);
    for(uint16_t colIdx = 0u; colIdx < table->numCols; colIdx++)
    {
        context.renderedCols[colIdx].width = colWidths[colIdx];
    }
    free(colWidths);

    uint16_t* rowHeights = calloc(table->numRows, sizeof(uint16_t));
    calculateRowHeights(&context, rowHeights);
    for(uint16_t rowIdx = 0u; rowIdx < table->numRows; rowIdx++)
    {
        context.renderedRows[rowIdx].height = rowHeights[rowIdx];
    }
    free(rowHeights);

    return context;
}

static void destroyRendererContext(RendererContext* ctx)
{
    free(ctx->renderedRows);
    free(ctx->renderedCols);
    for(uint16_t i = 0; i < ctx->table->numRows * ctx->table->numCols; i++)
    {
        destroyLineReader(&ctx->renderedCells[i].contentReader);
    }
    free(ctx->renderedCells);
}

static const char* getIntersectionChar(const RendererContext* ctx, uint16_t rowIdx, uint16_t colIdx)
{
    TableBorderStyle nStyle, eStyle, sStyle, wStyle;
    nStyle                            = (rowIdx == 0) ? TABLE_BORDER_NONE : getEffectiveStyle(ctx->table, rowIdx - 1, colIdx).borderLeft;
    eStyle                            = getEffectiveStyle(ctx->table, rowIdx, colIdx).borderTop;
    sStyle                            = getEffectiveStyle(ctx->table, rowIdx, colIdx).borderLeft;
    wStyle                            = (colIdx == 0) ? TABLE_BORDER_NONE : getEffectiveStyle(ctx->table, rowIdx, colIdx - 1).borderTop;
    TableBorderStyle usedBorderStyle  = (nStyle != TABLE_BORDER_NONE) ? nStyle : ((eStyle != TABLE_BORDER_NONE) ? eStyle : ((sStyle != TABLE_BORDER_NONE) ? sStyle : wStyle));
    size_t           index            = (nStyle != TABLE_BORDER_NONE ? 8 : 0) | (eStyle != TABLE_BORDER_NONE ? 4 : 0) | (sStyle != TABLE_BORDER_NONE ? 2 : 0) | (wStyle != TABLE_BORDER_NONE ? 1 : 0);
    return borders[usedBorderStyle][borderLookup[index]];
}

static void printCellLine(const RendererContext* ctx, uint16_t rowIdx, uint16_t colIdx, uint16_t line, FILE* stream)
{
    uint16_t            totalWidth   = 0;
    uint16_t            totalHeight  = 0;
    const Cell*         cell         = getCell(ctx->table, rowIdx, colIdx);
    const RenderedCell* renderedCell = getRenderedCell(ctx, rowIdx, colIdx);
    for(uint16_t c = colIdx; c < colIdx + cell->colSpan; c++)
    {
        totalWidth += ctx->renderedCols[c].width;
    }
    for(uint16_t r = rowIdx; r < rowIdx + cell->rowSpan; r++)
    {
        totalHeight += ctx->renderedRows[r].height;
    }

    uint16_t contentWidth  = renderedCell->contentWidth;
    uint16_t contentHeight = renderedCell->contentHeight;

    // Determine horizontal padding based on alignment
    uint16_t                 leftPadding  = 0;
    uint16_t                 rightPadding = 0;
    TableHorizontalAlignment hAlign       = getEffectiveStyle(ctx->table, rowIdx, colIdx).hAlign;
    if(hAlign == TABLE_H_ALIGN_LEFT)
    {
        rightPadding = totalWidth - contentWidth;
    }
    else if(hAlign == TABLE_H_ALIGN_RIGHT)
    {
        leftPadding = totalWidth - contentWidth;
    }
    else if(hAlign == TABLE_H_ALIGN_CENTER)
    {
        leftPadding  = (totalWidth - contentWidth) / 2;
        rightPadding = totalWidth - contentWidth - leftPadding;
    }

    // Determine vertical padding based on alignment
    uint16_t               topPadding = 0;
    TableVerticalAlignment vAlign     = getEffectiveStyle(ctx->table, rowIdx, colIdx).vAlign;
    if(vAlign == TABLE_V_ALIGN_TOP)
    {
        topPadding = 0;
    }
    else if(vAlign == TABLE_V_ALIGN_BOTTOM)
    {
        topPadding = totalHeight - contentHeight;
    }
    else if(vAlign == TABLE_V_ALIGN_MIDDLE)
    {
        topPadding = (totalHeight - contentHeight) / 2;
    }

    // Print line
    if(line < topPadding || line >= (topPadding + contentHeight))
    {
        // Print empty line
        for(uint16_t i = 0; i < totalWidth; i++)
        {
            fprintf(stream, " ");
        }
    }
    else
    {
        // Print content line with padding
        const char* str = getLine(&renderedCell->contentReader, line - topPadding);
        for(uint16_t i = 0; i < leftPadding; i++)
        {
            fprintf(stream, " ");
        }
        fprintf(stream, "%s", str);
        for(uint16_t i = 0; i < rightPadding; i++)
        {
            fprintf(stream, " ");
        }
    }
}

static void render(const RendererContext* ctx, FILE* stream)
{
    for(uint16_t r = 0; r < ctx->table->numRows; r++)
    {
        bool hasTopBorder = ctx->renderedRows[r].hasTopBorder;
        for(uint16_t c = 0; c < ctx->table->numCols; c++)
        {
            Cell* cell = getCell(ctx->table, r, c);
            if(cell->hasParent && (cell->parentX != c)) // Skip cells with colSpan
            {
                continue;
            }
            bool hasLeftBorder = ctx->renderedCols[c].hasLeftBorder;

            // Print horizontal border if needed
            const TableStyle style = getEffectiveStyle(ctx->table, r, c);

            if(hasTopBorder && hasLeftBorder)
            {
                fprintf(stream, "%s", getIntersectionChar(ctx, r, c));
            }
            if(hasTopBorder)
            {
                for(uint16_t i = 0; i < ctx->renderedCols[c].width; i++)
                {
                    fprintf(stream, "%s", borders[style.borderTop][9]);
                }
            }

            // Print row content
            for(uint16_t l = 0; l < ctx->renderedRows[r].height; l++)
            {
                if(hasLeftBorder)
                {
                    fprintf(stream, "%s", borders[style.borderLeft][10]);
                }
                printCellLine(ctx, r, c, getRenderedCell(ctx, r, c)->currLine++, stream);
            }
        }
        fprintf(stream, "\n");
    }
}


//***************************************************************************//
//************************** PUBLIC FUNCTION DEFINITIONS ********************//
//***************************************************************************//

void tableRenderer_render(const Table* table, FILE* stream)
{
    RendererContext ctx = createRendererContext(table);
    render(&ctx, stream);
    destroyRendererContext(&ctx);
}
