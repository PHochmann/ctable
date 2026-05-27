/**
 * @file table.h
 */


#pragma once

//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

//***************************************************************************//
//************************** PUBLIC TYPEDEFS ********************************//
//***************************************************************************//

/**
 * @brief Vertical alignment values for cell content.
 */
typedef enum
{
    TABLE_V_ALIGN_TOP,
    TABLE_V_ALIGN_MIDDLE,
    TABLE_V_ALIGN_BOTTOM
} TableVerticalAlignment;

/**
 * @brief Horizontal alignment values for cell content.
 */
typedef enum
{
    TABLE_H_ALIGN_LEFT,
    TABLE_H_ALIGN_CENTER,
    TABLE_H_ALIGN_RIGHT
} TableHorizontalAlignment;

/**
 * @brief Border line style values.
 */
typedef enum
{
    TABLE_BORDER_NONE,
    TABLE_BORDER_SOLID,
    TABLE_BORDER_DOUBLE,
    TABLE_BORDER_DASHED,
} TableBorderStyle;

/**
 * @brief Opaque table type.
 */
typedef struct Table Table;

/**
 * @brief Opaque style handle type.
 */
typedef struct TableStyle TableStyle;

//***************************************************************************//
//************************** PUBLIC FUNCTION DECLARATIONS *******************//
//***************************************************************************//

/* Table creation, printing, deletion */

/**
 * @brief Creates a new table with the specified number of columns.
 * @param numColumns The number of columns for the new table.
 * @return A pointer to the newly created table, or NULL if creation failed.
 */
Table* table_create(uint16_t numColumns);

/**
 * @brief Destroys a table and frees all associated memory.
 * @param table The table to destroy. If NULL, this function does nothing.
 * @note  After calling this function, the table pointer and all TableStyle pointers obtained from this table become invalid.
 */
void table_destroy(Table* table);

/**
 * @brief Prints the table to the standard output.
 * @param table The table to print.
 */
void table_print(const Table* table);

/**
 * @brief Prints the table to a specified file stream.
 * @param table The table to print.
 * @param stream The file stream to print to (e.g., stdout, stderr, or a file opened with fopen).
 */
void table_fprint(const Table* table, FILE* stream);

/* Style management */

/**
 * @brief       Retrieves the global style handle for the table.
 * @param table The table for which to retrieve the global style.
 * @return      A pointer to the TableStyle representing the global style of the table, or NULL if an error occurs.
 * @details     Modifying the global style will affect all cells that do not have a more specific style set (global < col < row < cell).
 *              The returned TableStyle is valid until table_destroy() is called and should not be freed by the caller.
 */
TableStyle* table_getDefaultStyle(Table* table);

/**
 * @brief       Retrieves the style handle for a specific row in the table.
 * @param table The table containing the row.
 * @param row   The index of the row (0-based). The row will be created if it does not already exist.
 * @return      A pointer to the TableStyle for the specified row, or NULL if an error occurs
 * @details     Modifying the row style will affect all cells in the specified row that do not have a more specific style set (global < col < row < cell).
 *              The returned TableStyle is valid until table_destroy() is called and should not be freed by the caller.
 */
TableStyle* table_getRowStyle(Table* table, uint16_t rowIdx);

/**
 * @brief       Retrieves the style handle for a specific column in the table.
 * @param table The table containing the column.
 * @param col   The index of the column (0-based). Must be less than the number of columns specified when the table was created.
 * @return      A pointer to the TableStyle for the specified column, or NULL if `col` is out of range or an error occurs
 * @details     Modifying the column style will affect all cells in the specified column that do not have a more specific style set (global < col < row < cell).
 *              The returned TableStyle is valid until table_destroy() is called and should not be freed by the caller.
 */
TableStyle* table_getColStyle(Table* table, uint16_t colIdx);

/**
 * @brief       Retrieves the style handle for a specific cell in the table.
 * @param table The table containing the cell.
 * @param row   The index of the row (0-based). The row will be created if it does not already exist.
 * @param col   The index of the column (0-based). Must be less than the number of columns specified when the table was created.
 * @return      A pointer to the TableStyle for the specified cell, or NULL if `col` is out of range or an error occurs.
 * @details     Modifying the cell style will affect the specified cell (global < col < row < cell).
 *              The returned TableStyle is valid until table_destroy() is called and should not be freed by the caller.
 */
TableStyle* table_getCellStyle(Table* table, uint16_t rowIdx, uint16_t colIdx);

/**
 * @brief          Sets the vertical alignment for a style handle.
 * @param styleRef The style handle to modify (obtained from table_get*Style(),
 * @param vAlign   The vertical alignment to set.
 */
void table_setVAlign(TableStyle* styleRef, TableVerticalAlignment vAlign);

/**
 * @brief          Sets the horizontal alignment for a style handle.
 * @param styleRef The style handle to modify (obtained from table_get*Style()),
 * @param hAlign   The horizontal alignment to set.
 */
void table_setHAlign(TableStyle* styleRef, TableHorizontalAlignment hAlign);


/**
 * @brief          Sets the left border style for a style handle.
 * @param styleRef The style handle to modify (obtained from table_get*Style()),
 * @param borderLeft The left border style to set.
 */
void table_setBorderLeft(TableStyle* styleRef, TableBorderStyle borderLeft);

/**
 * @brief          Sets the top border style for a style handle.
 * @param styleRef The style handle to modify (obtained from table_get*Style()),
 * @param borderTop The top border style to set.
 */
void table_setBorderTop(TableStyle* styleRef, TableBorderStyle borderTop);

/* Cell content management */

/**
 * @brief       Sets the text content of a specific cell in the table, using printf-style formatting.
 * @param table The table containing the cell.
 * @param rowIdx   The index of the row (0-based). The row will be created if it does not already exist.
 * @param colIdx   The index of the column (0-based). Must be less than the number of columns specified when the table was created.
 * @param fmt   The printf-style format string for the cell content, followed by any additional arguments required by the format string.
 * @return      true if the cell text was successfully set, false if `colIdx` is out of range, the cell already contains text, or an error occurs (e.g., memory allocation failure).
 */
bool table_setCellText(Table* table, uint16_t rowIdx, uint16_t colIdx, const char* fmt, ...);

/**
 * @brief The same as table_setCellText() but takes a va_list instead of variadic arguments.
 */
bool table_vsetCellText(Table* table, uint16_t rowIdx, uint16_t colIdx, const char* fmt, va_list args);

/**
 * @brief       Sets the minimum width for a specific column in the table.
 * @param table The table containing the column.
 * @param colIdx   The index of the column (0-based). Must be less than the number of columns specified when the table was created.
 * @param width The minimum width to set for the column
 */
void table_setMinColWidth(Table* table, uint16_t colIdx, uint16_t width);

/**
 * @brief       Sets the minimum height for a specific row in the table.
 * @param table The table containing the row.
 * @param rowIdx   The index of the row (0-based). The row will be created if it does not already exist.
 * @param height The minimum height to set for the row
 */
void table_setMinRowHeight(Table* table, uint16_t rowIdx, uint16_t height);

/**
 * @brief            Sets the span for a specific cell in the table.
 * @details          The cell will cover a rectangular area merging rows and/or columns.
 *                   `rowIdx` and `colIdx` define the top-left corner of the area. The size of the area is defined by `spanWidth` and `spanHeight` in terms of number of columns and rows, respectively.
 *                   Covered cells will not be printed. TableStyle pointers obtained for covered cells remain valid but will not affect the appearance of the table.
 *                   The style of the spanning cell is determined by the normal style resolution rules (global < col < row < cell) for the top-left corner of the spanned area, e.g. the cell at (`rowIdx`, `colIdx`).
 * @param table      The table containing the cell.
 * @param rowIdx        The index of the row (0-based). The row will be created if it does not already exist.
 * @param colIdx        The index of the column (0-based). Must be less than the number of columns specified when the table was created.
 * @param spanWidth  The number of columns the cell should span. Must be at least 1 and such that `colIdx + spanWidth` does not exceed the number of columns in the table.
 * @param spanHeight The number of rows the cell should span. Must be at least 1. There is no maximum limit for `spanHeight`, as rows will be created as needed to accommodate the span height.
 * @return           true if the cell span was successfully set, false if:
 *                   - The number of columns is exceeded
 *                   - Covered cells already contain text
 *                   - An error occurs (e.g., memory allocation failure).
 */
bool table_setCellSpan(Table* table, uint16_t rowIdx, uint16_t colIdx, uint16_t spanWidth, uint16_t spanHeight);

/**
 * @brief                     Sets a custom function for calculating the display width of strings in the table.
 * @param table               The table for which to set the string width function.
 * @param stringWidthFunction A pointer to a function that takes a null-terminated string as input and returns the display width of the string as an integer.
 *                            Cell content strings (raw, including newline characters) will be passed to this function to determine their display width for alignment and column width calculations.
 *                            If NULL is passed, the default string width calculation will be used.
 */
void table_setStringWidthFunction(Table* table, uint16_t (*stringWidthFunction)(const char*));

/* Cursor functions */

/**
 * @brief       Sets the current cell cursor position in the table.
 * @param table The table for which to set the cursor position.
 * @param rowIdx   The index of the row (0-based) to set the cursor to. The row will be created if it does not already exist.
 * @param colIdx   The index of the column (0-based) to set the cursor to. Must be less than the number of columns specified when the table was created.
 * @return      true if the cursor position was successfully set, false if `colIdx` is out of range or an error occurs (e.g., memory allocation failure).
 */
bool table_setCursor(Table* table, uint16_t rowIdx, uint16_t colIdx);

/**
 * @brief   Sets the text of the cell at the current cursor position and advances the cursor to the next cell.
 * @details The cursor moves to the next column in the same row.
 *          If the cursor is at the end of a row, it moves to the first column of the next row (creating a new row if necessary).
 * @param table The table for which to set the current cell text and advance the cursor.
 * @param fmt   The printf-style format string for the cell content, followed by any additional arguments required by the format string.
 * @return      true if success, false if an error occurs (e.g., memory allocation failure).
 */
bool table_setCurrentCellText(Table* table, const char* fmt, ...);

/**
 * @brief       Moves the cursor to the next row in the table.
 * @param table The table for which to move the cursor to the next row.
 */
void table_nextRow(Table* table);

/**
 * @brief       Retrieves the style handle for the cell at the current cursor position.
 * @details     Same as table_getCellStyle() for the cell at the current cursor position.
 */
TableStyle* table_getCurrentCellStyle(Table* table);

/**
 * @brief       Retrieves the style handle for the current row.
 * @details     Same as table_getRowStyle() for the current row.
 */
TableStyle* table_getCurrentRowStyle(Table* table);

/**
 * @brief       Retrieves the style handle for the current column.
 * @details     Same as table_getColStyle() for the current column.
 */
TableStyle* table_getCurrentColStyle(Table* table);
