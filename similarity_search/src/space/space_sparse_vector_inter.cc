/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
#include <random>

#include "space/space_sparse_vector_inter.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"

namespace similarity {

template <typename dist_t>
void
SpaceSparseVectorInter<dist_t>::CreateDenseVectFromObj(const Object* obj, 
                                                       dist_t* pVect,
                                                       size_t nElem) const {
  static std::hash<size_t>   indexHash;
  fill(pVect, pVect + nElem, static_cast<dist_t>(0));

  vector<SparseVectElem<dist_t>> target;
  UnpackSparseElements(obj->data(), obj->datalength(), target);

  for (SparseVectElem<dist_t> e: target) {
    size_t idx = indexHash(e.id_) % nElem;
    pVect[idx] += e.val_;
  }
}

template <typename dist_t>
void
SpaceSparseVectorInter<dist_t>::CreateVectFromObj(const Object* obj, vector<ElemType>& v) const {
  UnpackSparseElements(obj->data(), obj->datalength(), v);
}

template <typename dist_t>
Object* SpaceSparseVectorInter<dist_t>::CreateObjFromVect(IdType id, LabelType label, const std::vector<ElemType>& InpVect) const {
  char    *pData = NULL;
  size_t  dataLen = 0;

  try {
    PackSparseElements(InpVect, pData, dataLen);
    return new Object(id, label, dataLen, pData);
  } catch (...) {
    delete [] pData;
    throw;
  }
}

template <typename dist_t>
size_t
SpaceSparseVectorInter<dist_t>::ComputeOverlap(const Object* obj1, 
                                               const Object* obj2) const {
  vector<SparseVectElem<dist_t>> elems1, elems2;
  UnpackSparseElements(obj1->data(), obj1->datalength(), elems1);
  UnpackSparseElements(obj2->data(), obj2->datalength(), elems2);
  vector<IdType> ids1, ids2;
  for (SparseVectElem<dist_t> e: elems1)
    ids1.push_back(e.id_);
  for (SparseVectElem<dist_t> e: elems2)
    ids2.push_back(e.id_);
  return IntersectSizeScalarFast(&ids1[0], ids1.size(), &ids2[0], ids2.size());

}

template <typename dist_t>
OverlapInfo 
SpaceSparseVectorInter<dist_t>::ComputeOverlapInfo(const Object* objA, const Object* objB) const {
  vector<SparseVectElem<dist_t>> elemsA, elemsB;
  UnpackSparseElements(objA->data(), objA->datalength(), elemsA);
  UnpackSparseElements(objB->data(), objB->datalength(), elemsB);
  OverlapInfo res;

  float norm_left = 0;
  {
    for (uint32_t A = 0; A < elemsA.size(); ++A)
      norm_left += elemsA[A].val_*elemsA[A].val_;
    norm_left = sqrt(norm_left);
  }

  float norm_right = 0;
  {
    for (uint32_t B = 0; B < elemsB.size(); ++B)
      norm_right += elemsB[B].val_*elemsB[B].val_;
    norm_right = sqrt(norm_right);
  }
   
  uint32_t A = 0, B = 0;

  while (A < elemsA.size() && B < elemsA.size()) {
    uint32_t idA = elemsA[A].id_;
    uint32_t idB = elemsB[B].id_;
    if (idA < idB) {
      res.diff_sum_left_norm_ += elemsA[A].val_; 
      A++;
    } else if (idA > idB) {
      res.diff_sum_right_norm_ = elemsB[B].val_;
      B++;
    } else {
    // *A == *B
      res.overlap_dotprod_norm_ += elemsA[A].val_*elemsB[B].val_; 
      res.overlap_sum_left_norm_ += elemsA[A].val_; 
      res.overlap_sum_right_norm_ = elemsB[B].val_;
      res.overlap_qty_++;
      A++; B++;
    }
  }

  while (A < elemsA.size()) {
    res.diff_sum_left_norm_ += elemsA[A].val_; 
    A++;
  }

  while (B < elemsB.size()) {
    res.diff_sum_right_norm_ = elemsB[B].val_;
    B++;
  }

  if (norm_left > 0) {
    float inv = 1.0/norm_left;
    res.overlap_sum_left_norm_ *= inv;
    res.diff_sum_left_norm_    *= inv;
    res.overlap_dotprod_norm_  *= inv;
  }
  if (norm_right > 0) {
    float inv = 1.0/norm_right;
    res.overlap_sum_right_norm_ *= inv;
    res.diff_sum_right_norm_    *= inv;
    res.overlap_dotprod_norm_   *= inv;
  }

  return res;
}

template <typename dist_t>
size_t
SpaceSparseVectorInter<dist_t>::ComputeOverlap(const Object* obj1, 
                                               const Object* obj2,
                                               const Object* obj3) const {
  vector<SparseVectElem<dist_t>> elems1, elems2, elems3;
  UnpackSparseElements(obj1->data(), obj1->datalength(), elems1);
  UnpackSparseElements(obj2->data(), obj2->datalength(), elems2);
  UnpackSparseElements(obj3->data(), obj3->datalength(), elems3);
  vector<IdType> ids1, ids2, ids3;
  for (SparseVectElem<dist_t> e: elems1)
    ids1.push_back(e.id_);
  for (SparseVectElem<dist_t> e: elems2)
    ids2.push_back(e.id_);
  for (SparseVectElem<dist_t> e: elems3)
    ids3.push_back(e.id_);
  return IntersectSizeScalar3way(&ids1[0], ids1.size(), &ids2[0], ids2.size(), &ids3[0], ids3.size());

} 

template <typename dist_t>
size_t
SpaceSparseVectorInter<dist_t>::GetElemQty(const Object* obj) const {
  vector<SparseVectElem<dist_t>> elems;
  UnpackSparseElements(obj->data(), obj->datalength(), elems);
  return elems.size();

} 

template class SpaceSparseVectorInter<float>;
template class SpaceSparseVectorInter<double>;

}  // namespace similarity
