/* Copyright (C) 2005-2008 Damien Stehle.
   Copyright (C) 2007 David Cade.
   Copyright (C) 2008-2011 Xavier Pujol.

   This file is part of fplll. fplll is free software: you
   can redistribute it and/or modify it under the terms of the GNU Lesser
   General Public License as published by the Free Software Foundation,
   either version 2.1 of the License, or (at your option) any later version.

   fplll is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with fplll. If not, see <http://www.gnu.org/licenses/>. */

#include <config.h>
#include "main.h"

template<class ZT>
int lll(Options& o, ZZ_mat<ZT>& b) {
  ZZ_mat<ZT> u, uInv;
  const char* format = o.output_format ? o.output_format : "b";
  int status, flags = 0;
  if (o.verbose) flags |= LLL_VERBOSE;
  if (o.early_red) flags |= LLL_EARLY_RED;
  if (o.siegel) flags |= LLL_SIEGEL;

  if (strchr(format, 'v') != NULL) {
    // LLL-reduction with transform and inverse transform
    status = lllReduction(b, u, uInv, o.delta, o.eta, o.method, o.float_type,
            o.precision, flags);
  }
  else if (strchr(format, 'u') != NULL) {
    // LLL-reduction with transform
    status = lllReduction(b, u, o.delta, o.eta, o.method, o.float_type,
            o.precision, flags);
  }
  else {
    status = lllReduction(b, o.delta, o.eta, o.method, o.float_type,
            o.precision, flags);
  }

  for (int i = 0; format[i]; i++) {
    switch (format[i]) {
      case 'b': cout << b << endl; break;
      case 'u': cout << u << endl; break;
      case 'v': cout << uInv << endl; break;
      case 't': cout << status << endl; break;
      case ' ': cout << endl; break;
    }
  }
  if (status != RED_SUCCESS) {
    cerr << "Failure: " << getRedStatusStr(status) << endl;
  }
  return status;
}

/* BKZ reduction */

void read_pruning_vector(const char* file_name, Pruning& pr, int n) {
  double x;
  FILE* file = fopen(file_name, "r");
  CHECK(file, "Cannot open '" << file_name << "'");

  pr.coefficients.clear();
  for (int i = 0; i <= n && fscanf(file, "%lf", &x) == 1; i++) {
    pr.coefficients.push_back(x);
    CHECK(x > 0 && x <= 1, "Number " << x << " in file '" << file_name
          << "' is not in the interval (0,1]");
    if (i == 0) {
      CHECK(x == 1, "The first number in file '" << file_name
            << "' should be 1");
    }
    else {
      CHECK(pr.coefficients[i] <= pr.coefficients[i - 1], "File '" << file_name
            << "' should contain a non-increasing sequence of numbers");
    }
  }
  CHECK(static_cast<int>(pr.coefficients.size()) == n, "File '" << file_name
        << "' should contain exactly " << n << " numbers");
}

template<class ZT>
int bkz(Options& o, ZZ_mat<ZT>& b) {
  ABORT_MSG("mpz required for BKZ");
}

template<>
int bkz(Options& o, IntMatrix& b) {
  CHECK(o.block_size > 0, "Option -b is missing");
  vector<Strategy> strategies;
  if (!o.bkz_strategy_file.empty())
  {
    strategies = load_strategies_json(o.bkz_strategy_file.c_str());
  }

  BKZParam param(o.block_size, strategies);
  IntMatrix u;
  const char* format = o.output_format ? o.output_format : "b";
  int status;

  param.delta = o.delta;
  param.flags = o.bkz_flags;

  if (o.bkz_flags & BKZ_DUMP_GSO)
    param.dump_gso_filename = o.bkz_dump_gso_filename;
  if (o.bkz_flags & BKZ_GH_BND)
    param.gh_factor = o.bkz_gh_factor;
  if (o.bkz_flags & BKZ_MAX_LOOPS)
    param.max_loops = o.bkz_max_loops;
  if (o.verbose) param.flags |= BKZ_VERBOSE;
  if (o.no_lll) param.flags |= BKZ_NO_LLL;

  status = bkzReduction(&b, strchr(format, 'u') ? &u : NULL, param, o.float_type, o.precision);

  for (int i = 0; format[i]; i++) {
    switch (format[i]) {
      case 'b': cout << b << endl; break;
      case 'u': cout << u << endl; break;
      case 't': cout << status << endl; break;
      case ' ': cout << endl; break;
    }
  }
  if (status != RED_SUCCESS) {
    cerr << "Failure: " << getRedStatusStr(status) << endl;
  }
  return status;
}

/* HKZ reduction
   Note: since we only force |mu_i,j| <= eta with eta > 0.5, the solution
   is not unique even for a generic matrix */

template<class ZT>
int hkz(Options& o, ZZ_mat<ZT>& b) {
  ABORT_MSG("mpz required for HKZ");
}

template<>
int hkz(Options& o, ZZ_mat<mpz_t>& b) {
  int flags = 0;
  if (o.verbose) flags |= HKZ_VERBOSE;
  int status = hkzReduction(b, flags, o.float_type, o.precision);
  cout << b << endl;
  if (status != RED_SUCCESS) {
    cerr << "Failure: " << getRedStatusStr(status) << endl;
  }
  return status;
}

/* Shortest vector problem and closest vector problem */

template<class ZT>
int svpcvp(Options& o, ZZ_mat<ZT>& b, const vector< Z_NR<ZT> >& target) {
  if (target.empty()) {
    ABORT_MSG("mpz required for SVP");
  }
  else {
    ABORT_MSG("mpz required for CVP");
  }
}

template<>
int svpcvp(Options& o, ZZ_mat<mpz_t>& b, const vector< Z_NR<mpz_t> >& target) {
  const char* format = o.output_format ? o.output_format : "s";
  IntVect solCoord;  // In the LLL-reduced basis
  IntVect solCoord2; // In the initial basis
  IntVect solution;
  IntMatrix u;
  bool withCoord = strchr(format, 'c') != NULL;
  bool withCoordStd = strchr(format, 's') != NULL;
  int flags = SVP_DEFAULT | (o.verbose ? SVP_VERBOSE : 0);
  int flagsLLL = LLL_DEFAULT | (o.verbose ? LLL_VERBOSE : 0);
  int status;

  if (!o.no_lll) {
    if (withCoord) {
      status = lllReduction(b, u, LLL_DEF_DELTA, LLL_DEF_ETA, LM_WRAPPER,
                            FT_DEFAULT, 0, flagsLLL);
    }
    else {
      status = lllReduction(b, LLL_DEF_DELTA, LLL_DEF_ETA, LM_WRAPPER,
                            FT_DEFAULT, 0, flagsLLL);
    }
    if (status != RED_SUCCESS) {
      cerr << "LLL reduction failed: " << getRedStatusStr(status) << endl;
      return status;
    }
  }

  if (target.empty())
    status = shortestVector(b, solCoord, SVPM_PROVED, flags);
  else
    status = closestVector(b, target, solCoord, flags);

  if (status != RED_SUCCESS) {
    cerr << "Failure: " << getRedStatusStr(status) << endl;
    return status;
  }
  if (withCoord) {
    if (o.no_lll)
      solCoord2 = solCoord;
    else
      vectMatrixProduct(solCoord2, solCoord, u);
  }
  if (withCoordStd) {
    vectMatrixProduct(solution, solCoord, b);
  }

  for (int i = 0; format[i]; i++) {
    switch (format[i]) {
      case 'c': cout << solCoord2 << endl; break;
      case 's': cout << solution << endl; break;
      case 't': cout << status << endl; break;
      case ' ': cout << endl; break;
    }
  }
  return status;
}

template<class ZT>
int runAction(Options& o) {
  istream* is;
  ZZ_mat<ZT> m;
  vector<Z_NR<ZT> > target;

  if (o.input_file)
    is = new ifstream(o.input_file);
  else
    is = &cin;

  *is >> m;
  if (o.action == ACTION_CVP) {
    *is >> target;
  }
  if (!*is) ABORT_MSG("invalid input");
  if (o.input_file) delete is;

  int result = 0;
  switch (o.action) {
    case ACTION_LLL: result = lll(o, m); break;
    case ACTION_SVP: result = svpcvp(o, m, target); break;
    case ACTION_CVP: result = svpcvp(o, m, target); break;
    case ACTION_HKZ: result = hkz(o, m); break;
    case ACTION_BKZ: result = bkz(o, m); break;
    default: ABORT_MSG("unimplemented action"); break;
  }
  return result;
}

/* Command line parsing */

void readOptions(int argc, char** argv, Options& o) {
  for (int ac = 1; ac < argc; ac++) {
    if (strcmp(argv[ac], "-a") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after -a switch");
      if (strcmp(argv[ac], "lll") == 0)
        o.action = ACTION_LLL;
      else if (strcmp(argv[ac], "hkz") == 0)
        o.action = ACTION_HKZ;
      else if (strcmp(argv[ac], "bkz") == 0)
        o.action = ACTION_BKZ;
      else if (strcmp(argv[ac], "svp") == 0)
        o.action = ACTION_SVP;
      else if (strcmp(argv[ac], "cvp") == 0)
        o.action = ACTION_CVP;
      else
        ABORT_MSG("parse error in -a switch: lll or svp expected");
    }
    else if (strcmp(argv[ac], "-b") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after -b switch");
      o.block_size = atoi(argv[ac]);
    }
    else if (strcmp(argv[ac], "-bkzboundedlll") == 0) {
      o.bkz_flags |= BKZ_BOUNDED_LLL;
    }
    else if (strcmp(argv[ac], "-bkzmaxloops") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after '-bkzmaxloops'");
      o.bkz_max_loops = atoi(argv[ac]);
      o.bkz_flags |= BKZ_MAX_LOOPS;
    }
    else if (strcmp(argv[ac], "-bkzmaxtime") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after '-bkzmaxtime'");
      o.bkz_max_time = atof(argv[ac]);
      o.bkz_flags |= BKZ_MAX_TIME;
    }
    else if (strcmp(argv[ac], "-bkzautoabort") == 0) {
      o.bkz_flags |= BKZ_AUTO_ABORT;
    }
    else if (strcmp(argv[ac], "-bkzdumpgso") == 0) {
      ++ac;
      CHECK(ac < argc, "missing filename after -bkzdumpgso switch");
      o.bkz_dump_gso_filename = argv[ac];
      o.bkz_flags |= BKZ_DUMP_GSO;
    }
    else if (strcmp(argv[ac], "-c") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after -c switch");
      //o.c=atoi(argv[ac]); // ignored (was the number of columns)
    }
    else if (strcmp(argv[ac], "-bkzghbound") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after '-bkzghbound'");
      o.bkz_gh_factor = atof(argv[ac]);
      o.bkz_flags |= BKZ_GH_BND;
    }
    else if (strcmp(argv[ac], "-d") == 0 || strcmp(argv[ac], "-delta") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after -d switch");
      o.delta=atof(argv[ac]);
    }
    else if (strcmp(argv[ac], "-e") == 0 || strcmp(argv[ac], "-eta") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after -e switch");
      o.eta=atof(argv[ac]);
    }
    else if (strcmp(argv[ac], "-f") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after -f switch");
      if (strcmp("mpfr",argv[ac])==0)
        o.float_type = FT_MPFR;
      else if (strcmp("dpe",argv[ac])==0)
        o.float_type = FT_DPE;
      else if (strcmp("dd",argv[ac])==0)
        o.float_type = FT_DD;
      else if (strcmp("qd",argv[ac])==0)
        o.float_type = FT_QD;
      else if (strcmp("double",argv[ac])==0)
        o.float_type = FT_DOUBLE;
      else if (strcmp("longdouble", argv[ac]) == 0)
        o.float_type = FT_LONG_DOUBLE;
      else
        ABORT_MSG("parse error in -f switch : mpfr, qd, dd, dpe or double expected");
    }
    else if (strcmp(argv[ac], "-s") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after -s switch");
      o.bkz_strategy_file = argv[ac];
    }
    else if (strcmp(argv[ac], "-l") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after -l switch");
      o.siegel = !atoi(argv[ac]);
    }
    else if (strcmp(argv[ac], "-m") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after -m switch");
      if (strcmp("wrapper",argv[ac])==0)
        o.method = LM_WRAPPER;
      else if (strcmp("proved",argv[ac])==0)
        o.method = LM_PROVED;
      else if (strcmp("heuristic",argv[ac])==0)
        o.method = LM_HEURISTIC;
      else if (strcmp("fast",argv[ac])==0)
        o.method = LM_FAST;
      else if (strcmp("fastearly",argv[ac])==0) {
        o.method = LM_FAST;
        o.early_red = true;
      }
      else if (strcmp("heuristicearly",argv[ac])==0) {
        o.method = LM_HEURISTIC;
        o.early_red = true;
      }
      else
        ABORT_MSG("parse error in -m switch : proved, heuristic, fast, "
                  << "or wrapper expected");
    }
    else if (strcmp(argv[ac], "-nolll") == 0) {
      o.no_lll = true;
    }
    else if (strcmp(argv[ac], "-of") == 0) {
      ac++;
      CHECK(ac < argc, "missing value after -of switch");
      o.output_format = argv[ac];
    }
    else if (strcmp(argv[ac], "-p") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after -p switch");
      o.precision = atoi(argv[ac]);
    }
    else if (strcmp(argv[ac], "-r") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after -r switch");
      //o.r = atoi(argv[ac]); // ignored (was the number of rows)
    }
    else if (strcmp(argv[ac], "-v") == 0) {
      o.verbose = true;
    }
    else if (strcmp(argv[ac], "-y") == 0) {
      o.early_red = true;
    }
    else if (strcmp(argv[ac], "-z") == 0) {
      ++ac;
      CHECK(ac < argc, "missing value after -z switch");
      if (strcmp("mpz",argv[ac])==0)
        o.int_type = ZT_MPZ;
      else if (strcmp("long", argv[ac]) == 0 || strcmp("int", argv[ac]) == 0)
        o.int_type = ZT_LONG;
      else if (strcmp("double",argv[ac])==0)
        o.int_type = ZT_DOUBLE;
      else
        ABORT_MSG("parse error in -z switch : int, double or mpz expected");
    }
    else if ((strcmp(argv[ac], "-h") == 0) || (strcmp(argv[ac], "--help") == 0)) {
      cout << "Usage: " << argv[0] << " [options] [file]\n"
           << "List of options:\n"
           << "  -a [lll|svp]\n"
           << "       lll = LLL-reduce the input matrix (default)\n"
           << "       bkz = BKZ-reduce the input matrix\n"
           << "       svp = compute a shortest non-zero vector of the lattice\n"
           << "  -m [proved|heuristic|fast|wrapper]\n"
           << "       LLL version (default: wrapper)\n"
           << "  -z [int|mpz|double]\n"
           << "       Integer type in LLL (default: mpz)\n"
           << "  -f [mpfr|qd|dd|dpe|double]\n"
           << "       Floating-point type in LLL (proved/heuristic method only; default: dpe)\n"
           << "  -p <precision>\n"
           << "       Floating-point precision (only with -f mpfr)\n"
           << "  -d <delta> (default=0.99)\n"
           << "  -e <eta> (default=0.51)\n"
           << "  -l <lovasz>\n"
           << "  -s <filename.json> load BKZ strategies from filename"
           << "  -y\n"
           << "       Enable early reduction\n"
           << "  -b <blocksize>\n"
           << "       Size of BKZ blocks\n"
           << "  -v\n"
           << "       Enable verbose mode\n";
      exit(0);
    }
    else if (strcmp(argv[ac], "--version") == 0) {
      cout << "fplll " << VERSION << endl
           << "Copyright 2005-2012 Damien Stehle, David Cade, Xavier Pujol." << endl
           << "fplll is free software. You can redistribute it and/or modify" << endl
           << "it under the terms of the GNU Lesser General Public License as published by" << endl
           << "the Free Software Foundation, either version 2.1 of the License, or" << endl
           << "(at your option) any later version." << endl;
      exit(0);
    }
    else if (argv[ac][0] == '-') {
      ABORT_MSG("invalid option '" << argv[ac] << "'.\n"
        "Try '" << argv[0] << " --help' for more information.");
    }
    else {
      CHECK(!o.input_file, "too many input files");
      o.input_file = argv[ac];
    }
  }
}


int main(int argc, char** argv) {
  int result;
  Options o;
  readOptions(argc, argv, o);
  IntMatrix::setPrintMode(MAT_PRINT_REGULAR);
  switch (o.int_type) {
    case ZT_MPZ:    result = runAction<mpz_t>(o); break;
#ifdef FPLLL_WITH_ZLONG
    case ZT_LONG:   result = runAction<long int>(o); break;
#endif
#ifdef FPLLL_WITH_ZDOUBLE
    case ZT_DOUBLE: result = runAction<double>(o); break;
#endif
    default:        ABORT_MSG("compiled without support for this integer type");
  }
  return result;
}