/**
 * @file SparseMatrix.cpp
 * @brief Sparse matrix functionality for iSAM.
 * @author Michael Kaess
 * @version $Id: SparseMatrix.cpp 2921 2010-08-27 04:23:38Z kaess $
 *
 * Copyright (C) 2009-2010 Massachusetts Institute of Technology.
 * Michael Kaess (kaess@mit.edu) and John J. Leonard (jleonard@mit.edu)
 *
 * This file is part of iSAM.
 *
 * iSAM is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * iSAM is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with iSAM.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <cstring> // memcpy()
#include <fstream>
#include <iostream>
#include <cmath>

#include "isam/util.h"

#include "isam/SparseMatrix.h"

using namespace std;

namespace isam {

// initial values don't matter much
const int MIN_NUM_COLS = 10;
const int MIN_NUM_ROWS = 10;

void SparseMatrix::_allocate_SparseMatrix(int num_rows, int num_cols, int max_num_rows, int max_num_cols, bool init_rows) {
  _num_rows = num_rows;
  _num_cols = num_cols;
  _max_num_rows = max_num_rows;
  _max_num_cols = max_num_cols;
  _rows = new SparseVector_p[_max_num_rows];
  if (init_rows) {
    for (int row=0; row<_num_rows; row++) {
      _rows[row] = new SparseVector();
    }
  }
}

void SparseMatrix::_copy_from_SparseMatrix(const SparseMatrix& mat) {
  _allocate_SparseMatrix(mat._num_rows, mat._num_cols, mat._num_rows, mat._num_cols, false);
  for (int row=0; row<_num_rows; row++) {
    _rows[row] = new SparseVector(*mat._rows[row]);
  }
}

void SparseMatrix::_dealloc_SparseMatrix() {
  for (int row=0; row<_num_rows; row++) {
    delete _rows[row];
    _rows[row] = NULL;
  }
  delete[] _rows;
  _rows = NULL;
}

SparseMatrix::SparseMatrix(int num_rows, int num_cols) {
  require(num_rows>=0 && num_cols>=0, "SparseMatrix constructor: Index out of range");
  // we reserve space for at least 2 times what we need now, but not to be less than MIN_COLS
  _allocate_SparseMatrix(num_rows, num_cols, max(MIN_NUM_ROWS, 2*num_rows), max(MIN_NUM_COLS, 2*num_cols));
}

SparseMatrix::SparseMatrix(const SparseMatrix& mat) {
  _copy_from_SparseMatrix(mat);
}

SparseMatrix::SparseMatrix(const SparseMatrix& mat, int num_rows, int num_cols, int first_row, int first_col) {
  _allocate_SparseMatrix(num_rows, num_cols, num_rows, num_cols);
  for (int row=0; row<num_rows; row++) {
    // copy parts of rowvectors
    *_rows[row] = SparseVector(*mat._rows[row+first_row], num_rows, first_row);
  }
}

SparseMatrix::SparseMatrix(int num_rows, int num_cols, SparseVector_p* rows) {
  _num_rows = num_rows;
  _num_cols = num_cols;
  _max_num_rows = num_rows;
  _max_num_cols = num_cols;
  _rows = new SparseVector_p[_max_num_rows];
  for (int row=0; row<_num_rows; row++) {
    _rows[row] = rows[row];
  }
}

SparseMatrix::~SparseMatrix() {
  _dealloc_SparseMatrix();
}

const SparseMatrix& SparseMatrix::operator= (const SparseMatrix& mat) {
  // sanity check
  if (this==&mat)
    return *this;

  // free old stuff
  _dealloc_SparseMatrix();

  // copy rhs
  _copy_from_SparseMatrix(mat);

  // return self
  return *this;
}

const double SparseMatrix::operator()(int row, int col) const {
  require((row>=0) && (row<_num_rows) && (col>=0) && (col<_num_cols),
          "SparseMatrix::operator(): Index out of range.");
  return (*_rows[row])(col);
}

void SparseMatrix::set(int row, int col, const double val, bool grow_matrix) {
  require(row>=0 && col>=0, "SparseMatrix::set: Invalid index.");
  if (grow_matrix) {
    ensure_num_rows(row+1);
    ensure_num_cols(col+1);
  } else {
    require(row<_num_rows && col<_num_cols, "SparseMatrix::set: Index out of range.");
  }
  _rows[row]->set(col, val);
}

void SparseMatrix::append_in_row(int row, int col,const double val) {
  require(row>=0 && col>=0 && row<_num_rows && col<_num_cols,
      "SparseMatrix::append_in_row: Index out of range.");
  _rows[row]->append(col, val);
}

int SparseMatrix::nnz() const {
  int nnz = 0;
  for (int row=0; row<_num_rows; row++) {
    nnz+=_rows[row]->nnz();
  }
  return nnz;
}

void SparseMatrix::print(ostream& out) const {
  out << "%triples: (" << _num_rows << "x" << _num_cols << ", nnz:" << nnz() << ")" << endl;
  out.precision(12);
  for (int row=0; row<_num_rows; row++) {
    for (SparseVectorIter iter(*_rows[row]); iter.valid(); iter.next()) {
      double val;
      int col = iter.get(val);
      out << row << " " << col << " " << val << endl;
    }
  }
}

void SparseMatrix::print(const string& file_name) const {
  ofstream out(file_name.c_str());
  print(out);
  out.close();
}

void SparseMatrix::print_stats() const {
  cout << num_rows() << "x" << num_cols() << " nnz:" << nnz() << endl;
}

void SparseMatrix::print_pattern() const {
  for (int row=0; row<_num_rows; row++) {
    for (int col=0; col<_num_cols; col++) {
      double val = operator()(row, col);
      if (val!=0) {
        cout << "x";
      } else {
        cout << ".";
      }
    }
    cout << endl;
  }
}

const SparseVector& SparseMatrix::get_row(int row) const {
  require(row>=0 && row<_num_rows, "SparseMatrix::get_row: Index out of range.");
  return *_rows[row];
}

void SparseMatrix::set_row(int row, const SparseVector& new_row) {
  require(row>=0 && row<_num_rows, "SparseMatrix::set_row: Index out of range.");
  *_rows[row] = new_row;
}

void SparseMatrix::import_rows(int num_rows, int num_cols, SparseVector_p* rows) {
  _dealloc_SparseMatrix();
  _num_rows = num_rows;
  _num_cols = num_cols;
  _max_num_rows = num_rows;
  _max_num_cols = num_cols;
  _rows = new SparseVector_p[_max_num_rows];
  for (int row=0; row<_num_rows; row++) {
    _rows[row] = rows[row];
  }
}

void SparseMatrix::append_new_rows(int num) {
  require(num>=1, "SparseMatrix::append_new_rows: Cannot add less than one row.");
  int pos = _num_rows;
  if (_num_rows+num > _max_num_rows) {
    // automatic resizing, amortized cost O(1)
    int new_max_num_rows = max(2*_max_num_rows, _num_rows+num);
    SparseVector_p* new_rows = new SparseVector_p[new_max_num_rows];
    memcpy(new_rows, _rows, _num_rows*sizeof(SparseVector*));
    delete[] _rows;
    _max_num_rows = new_max_num_rows;
    _rows = new_rows;
  }
  for (int row=pos; row<=pos-1+num; row++) {
    _rows[row] = new SparseVector();
  }
  _num_rows += num;
}

void SparseMatrix::append_new_cols(int num) {
  require(num>=1, "SparseMatrix::append_new_cols: Cannot add less than one column.");
  if (_num_cols+num > _max_num_cols) {
    // this is actually used in OrderedSparseMatrix for the column translation table
    _max_num_cols = max(2*_max_num_cols, _num_cols+num);
  }
  _num_cols += num;
}

void SparseMatrix::ensure_num_rows(int num_rows) {
  require(num_rows>0, "SparseMatrix::ensure_num_rows: num_rows must be positive.");
  if (_num_rows < num_rows) {
    append_new_rows(num_rows - _num_rows);
  }
}

void SparseMatrix::ensure_num_cols(int num_cols) {
  require(num_cols>0, "SparseMatrix::ensure_num_cols: num_cols must be positive.");
  if (_num_cols < num_cols) {
    append_new_cols(num_cols - _num_cols);
  }
}

void SparseMatrix::remove_row() {
  require(_num_rows>0, "SparseMatrix::remove_row called on empty matrix.");
  // no need to worry about resizing _rows itself for this special case...
  delete _rows[_num_rows-1];
  _rows[_num_rows-1] = NULL;
  _num_rows--;
}

void SparseMatrix::apply_givens(int row, int col, double* c_givens, double* s_givens) {
  require(row>=0 && row<_num_rows && col>=0 && col<_num_cols, "SparseMatrix::apply_givens: index outside matrix.");
  require(row>col, "SparseMatrix::apply_givens: can only zero entries below the diagonal.");
  const SparseVector& row_top = *_rows[col];
  const SparseVector& row_bot = *_rows[row];
  double a = row_top(col);
  double b = row_bot(col);
  double c, s;
  givens(a, b, c, s);
  if (c_givens) *c_givens = c;
  if (s_givens) *s_givens = s;

  SparseVector new_row_top;
  SparseVector new_row_bot;
  SparseVectorIter iter_top(row_top);
  SparseVectorIter iter_bot(row_bot);
  bool top_valid = iter_top.valid();
  bool bot_valid = iter_bot.valid();
  while (top_valid || bot_valid) {
    double val_top = 0.;
    double val_bot = 0.;
    int idx_top = (top_valid)?iter_top.get(val_top):-1;
    int idx_bot = (bot_valid)?iter_bot.get(val_bot):-1;
    int idx;
    if (idx_bot<0) {
      idx = idx_top;
    } else if (idx_top<0) {
      idx = idx_bot;
    } else {
      idx = min(idx_top, idx_bot);
    }
    if (top_valid) {
      if (idx==idx_top) {
        iter_top.next();
      } else {
        val_top = 0.;
      }
    }
    if (bot_valid) {
      if (idx==idx_bot) {
        iter_bot.next();
      } else {
        val_bot = 0.;
      }
    }
    double new_val_top = c*val_top - s*val_bot;
    double new_val_bot = s*val_top + c*val_bot;
    // remove numerically zero values to keep sparsity
    if (fabs(new_val_top) >= NUMERICAL_ZERO) {
      // append for O(1) operation - even O(log n) is too
      // slow here, because this is called extremely often!
      new_row_top.append(idx, new_val_top);
    }
    if (fabs(new_val_bot) >= NUMERICAL_ZERO) {
      new_row_bot.append(idx, new_val_bot);
    }
    top_valid = iter_top.valid();
    bot_valid = iter_bot.valid();
  }

  *_rows[col] = new_row_top;
  *_rows[row] = new_row_bot;
  _rows[row]->remove(col); // by definition, this entry is exactly 0
}

int SparseMatrix::triangulate_with_givens() {
  int count = 0;
  for (int row=0; row<_num_rows; row++) {
    while (true) {
      int col = _rows[row]->first();
      if (col >= row || col < 0) {
        break;
      }
      apply_givens(row, col);
      count++;
    }
  }
  return count;
}

const Vector operator*(const SparseMatrix& lhs, const Vector& rhs) {
  require(lhs.num_cols()==rhs.num_rows(), "SparseMatrix::operator* matrix and vector incompatible.");
  Vector res(lhs.num_rows());
  for (int row=0; row<lhs.num_rows(); row++) {
    for (SparseVectorIter iter(lhs.get_row(row)); iter.valid(); iter.next()) {
      double val;
      int col = iter.get(val);
      res.set(row, res(row) + val * rhs(col));
    }
  }
  return res;
}

Vector mul_SparseMatrixTrans_Vector(const SparseMatrix& lhs, const Vector& rhs) {
  require(lhs.num_rows()==rhs.num_rows(), "SparseMatrix::mul_SparseMatrixTrans_Vector matrix and vector incompatible.");
  Vector res(lhs.num_cols());
  for (int row=0; row<lhs.num_rows(); row++) {
    for (SparseVectorIter iter(lhs.get_row(row)); iter.valid(); iter.next()) {
      double val;
      int col = iter.get(val);
      res.set(col, res(col) + val * rhs(row));
    }
  }
  return res;
}

SparseMatrix sparseMatrix_of_matrix(const Matrix& m) {
  SparseMatrix s(m.num_rows(), m.num_cols());
  for (int r=0; r<m.num_rows(); r++) {
    for (int c=0; c<m.num_cols(); c++) {
      s.set(r, c, m(r,c));
    }
  }
  return s;
}

Matrix matrix_of_sparseMatrix(const SparseMatrix& s) {
  Matrix m(s.num_rows(), s.num_cols());
  for (int r=0; r<s.num_rows(); r++) {
    for (int c=0; c<s.num_cols(); c++) {
      m.set(r, c, s(r,c));
    }
  }
  return m;
}

}
