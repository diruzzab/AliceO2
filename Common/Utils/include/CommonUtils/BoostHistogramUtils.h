// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   BoostHistogramUtils.h
/// \author Hannah Bossi, hannah.bossi@yale.edu

#ifndef ALICEO2_UTILS_BOOSTHISTOGRAMUTILS
#define ALICEO2_UTILS_BOOSTHISTOGRAMUTILS

#include <cmath>
#include <numeric>
#include <algorithm>
#include <vector>
#include <array>
#include <TH1.h>
#include <TH2.h>
#include <TFile.h>

#include "Framework/Logger.h"
#include "Rtypes.h"
#include "TLinearFitter.h"
#include "TVectorD.h"
#include "TF1.h"
#include "Foption.h"
#include "HFitInterface.h"
#include "TFitResultPtr.h"
#include "TFitResult.h"
#include "Fit/Fitter.h"
#include "Fit/BinData.h"
#include "Math/WrappedMultiTF1.h"
#include <boost/histogram.hpp>
#include <boost/histogram/histogram.hpp>
#include <boost/format.hpp>
#include <boost/histogram/axis.hpp>
#include <boost/histogram/make_histogram.hpp>
#include <boost/histogram/accumulators/mean.hpp>

using boostHisto2d = boost::histogram::histogram<std::tuple<boost::histogram::axis::regular<double, boost::use_default, boost::use_default, boost::use_default>, boost::histogram::axis::regular<double, boost::use_default, boost::use_default, boost::use_default>>, boost::histogram::unlimited_storage<std::allocator<char>>>;
using boostHisto1d = boost::histogram::histogram<std::tuple<boost::histogram::axis::regular<double, boost::use_default, boost::use_default, boost::use_default>>, boost::histogram::unlimited_storage<std::allocator<char>>>;

using boostHisto2d_VarAxis = boost::histogram::histogram<std::tuple<boost::histogram::axis::variable<double, boost::use_default, boost::use_default, std::allocator<double>>, boost::histogram::axis::variable<double, boost::use_default, boost::use_default, std::allocator<double>>>>;
using boostHisto1d_VarAxis = boost::histogram::histogram<std::tuple<boost::histogram::axis::variable<double, boost::use_default, boost::use_default, std::allocator<double>>>>;

namespace o2
{
namespace utils
{

/// \class BinCenterView
/// \brief  Axis iterator over bin centers.
template <typename AxisIterator>
class BinCenterView
{
 public:
  BinCenterView(AxisIterator iter) : mBaseIterator(iter) {}
  ~BinCenterView() = default;
  AxisIterator&
    operator++()
  {
    ++mBaseIterator;
    return mBaseIterator;
  }
  AxisIterator operator++(int)
  {
    AxisIterator result(mBaseIterator);
    mBaseIterator++;
    return result;
  }

  decltype(auto) operator*() { return mBaseIterator->center(); }

 private:
  AxisIterator mBaseIterator;
};

/// \class BinUpperView
/// \brief  Axis iterator over bin upper edges.
template <typename AxisIterator>
class BinUpperView
{
 public:
  BinUpperView(AxisIterator iter) : mBaseIterator(iter) {}
  ~BinUpperView() = default;
  AxisIterator&
    operator++()
  {
    ++mBaseIterator;
    return mBaseIterator;
  }
  AxisIterator operator++(int)
  {
    AxisIterator result(mBaseIterator);
    mBaseIterator++;
    return result;
  }

  decltype(auto) operator*() { return mBaseIterator->upper(); }

 private:
  AxisIterator mBaseIterator;
};

/// \class BinLowerView
/// \brief  Axis iterator over bin lower edges.
template <typename AxisIterator>
class BinLowerView
{
 public:
  BinLowerView(AxisIterator iter) : mBaseIterator(iter) {}
  ~BinLowerView() = default;
  AxisIterator&
    operator++()
  {
    ++mBaseIterator;
    return mBaseIterator;
  }
  AxisIterator operator++(int)
  {
    AxisIterator result(mBaseIterator);
    mBaseIterator++;
    return result;
  }

  decltype(auto) operator*() { return mBaseIterator->lower(); }

 private:
  AxisIterator mBaseIterator;
};

template <typename AxisIterator>
BinCenterView<AxisIterator> operator+(BinCenterView<AxisIterator> lhs, int n)
{
  BinCenterView<AxisIterator> result(lhs);
  for (int i = 0; i < n; i++) {
    ++result;
  }
  return result;
}

template <typename AxisIterator>
BinUpperView<AxisIterator> operator+(BinUpperView<AxisIterator> lhs, int n)
{
  BinUpperView<AxisIterator> result(lhs);
  for (int i = 0; i < n; i++) {
    ++result;
  }
  return result;
}

template <typename AxisIterator>
BinLowerView<AxisIterator> operator+(BinLowerView<AxisIterator> lhs, int n)
{
  BinLowerView<AxisIterator> result(lhs);
  for (int i = 0; i < n; i++) {
    ++result;
  }
  return result;
}

template <typename T, int nparams>
class fitResult
{
 public:
  fitResult() = default;

  fitResult(T chi2, const T (&list)[nparams]) : mChi2(chi2),
                                                mFitParameters()
  {
    memcpy(mFitParameters.data(), list, sizeof(T) * nparams);
  }

  /// \brief Get the paratmeters of a fit result. Ex:  result.getParameter<1>();
  template <int index>
  T getParameter() const
  {
    static_assert(index <= nparams, "Acessing invalid parameter");
    return mFitParameters[index];
  }

  /// \brief Set the paratmeters of a fit result. Ex:  result.setParameter<1>(T(value)));
  template <int index>
  void setParameter(T parameter)
  {
    static_assert(index <= nparams, "Attempting to set invalid parameter");
    mFitParameters[index] = parameter;
  }

  /// \brief Set the chi2 of the fit result.
  void setChi2(double chi2In)
  {
    mChi2 = chi2In;
  }
  T getChi2() const { return mChi2; }

 private:
  T mChi2;                               ///< chi2 of the fit
  std::array<T, nparams> mFitParameters; ///< parameters of the fit (0-Constant, 1-Mean, 2-Sigma,  3-Sum)
};

/**
 * \enum FitGausError_t
 * \brief Error code for invalid result in the fitGaus process
 */
enum class FitGausError_t {
  FIT_ERROR_MAX,        ///< Gaus fit failed! yMax too large
  FIT_ERROR_MIN,        ///< Gaus fit failed! yMax < 4
  FIT_ERROR_ENTRIES,    ///< Gaus fit failed! entries < 12
  FIT_ERROR_KTOL_MEAN,  ///< Gaus fit failed! std::abs(par[1]) < kTol
  FIT_ERROR_KTOL_SIGMA, ///< Gaus fit failed! std::abs(par[2]) < kTol
  FIT_ERROR_KTOL_RMS    ///< Gaus fit failed! RMS < kTol

};

/// \brief Printing an error message when then fit returns an invalid result
/// \param errorcode Error of the type FitGausError_t, thrown when fit result is invalid.
std::string createErrorMessageFitGaus(o2::utils::FitGausError_t errorcode);

/// \brief Function to fit histogram to a gaussian using iterators.
/// \param first begin iterator of the histogram
/// \param last end iterator of the histogram
/// \param axisFirst axis iterator over the bin centers
/// \param ignoreUnderOverflowBin switch to disable taking under and overflow bin into fit
/// \return result
///      result is of the type fitResult, which contains 4 parameters (0-Constant, 1-Mean, 2-Sigma,  3-Sum)
///
/// ** Temp Note: For now we forgo the templated struct in favor of a std::vector in order to
/// have this compile while we are working out the details
template <typename T, typename Iterator, typename BinCenterView>
std::vector<double> fitGaus(Iterator first, Iterator last, BinCenterView axisfirst, const bool ignoreUnderOverflowBin = true)
{

  if (ignoreUnderOverflowBin) {
    first++;
    last--;
    axisfirst++;
  }

  TLinearFitter fitter(3, "pol2");
  TMatrixD mat(3, 3);
  double kTol = mat.GetTol();
  fitter.StoreData(kFALSE);
  fitter.ClearPoints();
  TVectorD par(3);
  TMatrixD A(3, 3);
  TMatrixD b(3, 1);

  // return type of std::max_element is an iterator, cannot cast implicitly to double
  // pointer needs to be dereferenced afterwards
  auto yMax = std::max_element(first, last);
  auto nbins = std::distance(first, last);
  auto getBinWidth = [](BinCenterView axisiter) {
    double binCenter1 = *axisiter;
    axisiter++;
    double binCenter2 = *axisiter;
    return std::abs(binCenter1 - binCenter2);
  };
  double binWidth = getBinWidth(axisfirst);

  float meanCOG = 0;
  float rms2COG = 0;
  float sumCOG = 0;

  float entries = 0;
  int nfilled = 0;

  for (auto iter = first; iter != last; iter++) {
    entries += *iter;
    if (*iter > 0) {
      nfilled++;
    }
  }

  if (*yMax < 4) {
    throw FitGausError_t::FIT_ERROR_MIN;
  }
  if (entries < 12) {
    throw FitGausError_t::FIT_ERROR_ENTRIES;
  }

  // create the result, first fill it with all 0's
  std::vector<double> result;
  for (int r = 0; r < 5; r++) {
    result.push_back(0);
  }
  // then set the third parameter to entries
  result.at(3) = entries;

  int npoints = 0;
  // in this loop: increase iter and axisiter (iterator for bin center and bincontent)
  auto axisiter = axisfirst;
  for (auto iter = first; iter != last; iter++, axisiter++) {
    if (nbins > 1) {
      // dont take bins with 0 entries or bins with nan into account
      // if y-value (*iter) is 1, log(*iter) will be 0. Exclude these cases
      if (isnan(*axisiter) || isinf(*axisiter) || *iter <= 0 || *iter == 1) {
        continue;
      }
      double x = *axisiter;
      // take logarithm of gaussian in order to obtain a pol2
      double y = std::log(*iter);
      // first order taylor series of log(x) to approimate the errors df(x)/dx * err(f(x))
      double ey = std::sqrt(fabs(*iter)) / fabs(*iter);

      fitter.AddPoint(&x, y, ey);
      if (npoints < 3) {
        A(npoints, 0) = 1;
        A(npoints, 1) = x;
        A(npoints, 2) = x * x;
        b(npoints, 0) = y;
        meanCOG += x * nbins;
        rms2COG += x * x * nbins;
        sumCOG += nbins;
      }
      npoints++;
    }
  }
  double chi2 = 0;
  if (npoints >= 3) {
    if (npoints == 3) {
      // analytic calculation of the parameters for three points
      A.Invert();
      TMatrixD res(1, 3);
      res.Mult(A, b);
      par[0] = res(0, 0);
      par[1] = res(0, 1);
      par[2] = res(0, 2);
      chi2 = -3.;
    } else {
      // use fitter for more than three points
      fitter.Eval();
      fitter.GetParameters(par);
      fitter.GetCovarianceMatrix(mat);
      result.at(4) = (fitter.GetChisquare() / double(npoints));
    }

    if (std::abs(par[1]) < kTol) {
      throw FitGausError_t::FIT_ERROR_KTOL_MEAN;
    }
    if (std::abs(par[2]) < kTol) {
      throw FitGausError_t::FIT_ERROR_KTOL_SIGMA;
    }

    // calculate parameters for gaus from pol2 fit
    T param1 = T(par[1] / (-2. * par[2]));
    result.at(1) = param1;
    result.at(2) = T(1. / std::sqrt(std::abs(-2. * par[2])));
    auto lnparam0 = par[0] - par[1] * par[1] / (4 * par[2]);
    if (lnparam0 > 307) {
      throw FitGausError_t::FIT_ERROR_MAX;
    }

    result.at(0) = T(std::exp(lnparam0));
    return result;

  } else if (npoints == 2) {
    // use center of gravity for 2 points
    meanCOG /= sumCOG;
    rms2COG /= sumCOG;

    result.at(0) = *yMax;
    result.at(1) = meanCOG;
    result.at(2) = std::sqrt(std::abs(meanCOG * meanCOG - rms2COG));
    result.at(4) = -2;
  } else if (npoints == 1) {
    meanCOG /= sumCOG;

    result.at(0) = *yMax;
    result.at(1) = meanCOG;
    result.at(2) = binWidth / std::sqrt(12);
    result.at(4) = -1;
  }

  return result;
}

template <typename valuetype, typename... axes>
std::vector<double> fitBoostHistoWithGaus(boost::histogram::histogram<axes...>& hist)
{
  return fitGaus<valuetype>(hist.begin(), hist.end(), BinCenterView(hist.axis(0).begin()));
}

/// \brief Convert a 1D root histogram to a Boost histogram
boostHisto1d_VarAxis boosthistoFromRoot_1D(TH1D* inHist1D);

/// \brief Convert a 2D root histogram to a Boost histogram
boostHisto2d_VarAxis boostHistoFromRoot_2D(TH2D* inHist2D);

/// \brief Convert a 2D boost histogram to a root histogram
template <class BoostHist>
TH1F TH1FFromBoost(BoostHist hist, const char* name = "hist")
{
  const int nbinsx = hist.axis(0).size();
  const double binxlow = hist.axis(0).bin(0).lower();
  const double binxhigh = hist.axis(0).bin(nbinsx - 1).upper();

  TH1F hRoot(name, name, nbinsx, binxlow, binxhigh);
  // trasfer the acutal values
  for (int x = 0; x < nbinsx; x++) {
    hRoot.SetBinContent(x + 1, hist.at(x));
  }
  return hRoot;
}

/// \brief Convert a 2D boost histogram to a root histogram
// template <typename valuetype, typename... axes>
template <class BoostHist>
TH2F TH2FFromBoost(BoostHist hist, const char* name = "hist")
{
  const int nbinsx = hist.axis(0).size();
  const int nbinsy = hist.axis(1).size();
  const double binxlow = hist.axis(0).bin(0).lower();
  const double binxhigh = hist.axis(0).bin(nbinsx - 1).upper();
  const double binylow = hist.axis(1).bin(0).lower();
  const double binyhigh = hist.axis(1).bin(nbinsy - 1).upper();

  TH2F hRoot(name, name, nbinsx, binxlow, binxhigh, nbinsy, binylow, binyhigh);
  // trasfer the acutal values
  for (int x = 0; x < nbinsx; x++) {
    for (int y = 0; y < nbinsy; y++) {
      hRoot.SetBinContent(x + 1, y + 1, hist.at(x, y));
    }
  }
  return hRoot;
}

/// \brief Function to project 2d boost histogram onto x-axis
/// \param hist2d 2d boost histogram
/// \param binLow lower bin in y for projection
/// \param binHigh lower bin in y for projection
/// \return result
///      1d boost histogram from projection of the input 2d boost histogram
template <typename... axes>
auto ProjectBoostHistoX(boost::histogram::histogram<axes...>& hist2d, const int binLow, const int binHigh)
{
  using namespace boost::histogram::literals; // enables _c suffix needed for projection

  unsigned int nbins = hist2d.axis(0).size();
  // make reduced histo in range that we want to project
  auto reducedHisto2d = boost::histogram::algorithm::reduce(hist2d, boost::histogram::algorithm::shrink(hist2d.axis(0).bin(0).lower(), hist2d.axis(0).bin(nbins - 1).upper()), boost::histogram::algorithm::shrink(binLow, binHigh));

  // set under and overflow bin to 0 such that they will not be used in the projection
  for (int i = 0; i < reducedHisto2d.axis(0).size(); ++i) {
    reducedHisto2d.at(i, -1) = 0;
    reducedHisto2d.at(i, reducedHisto2d.axis(1).size()) = 0;
  }
  // make the projection onto the x-axis (0_c) of the reduced histogram
  auto histoProj = boost::histogram::algorithm::project(reducedHisto2d, 0_c);

  return histoProj;
}

/// \brief Function to project 2d boost histogram onto x-axis
/// \param hist2d 2d boost histogram
/// \param binLow lower bin in y for projection
/// \param binHigh lower bin in y for projection
/// \return result
///      1d boost histogram from projection of the input 2d boost histogram
template <typename... axes>
auto ProjectBoostHistoXFast(boost::histogram::histogram<axes...>& hist2d, const int binLow, const int binHigh)
{
  unsigned int nbins = hist2d.axis(0).size();
  double binStartX = hist2d.axis(0).bin(0).lower();
  double binEndX = hist2d.axis(0).bin(nbins - 1).upper();
  auto histoProj = boost::histogram::make_histogram(boost::histogram::axis::regular<>(nbins, binStartX, binEndX));

  // Now rewrite the bin content of the 1d histogram to get the summed bin content in the specified range
  for (int x = 0; x < nbins; ++x) {
    histoProj.at(x) = 0;
    for (int y = binLow; y < binHigh; ++y) {
      histoProj.at(x) = histoProj.at(x) + hist2d.at(x, y);
    }
  }

  return histoProj;
}

} // end namespace utils
} // end namespace o2

#endif
