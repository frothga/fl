/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/cluster.h"
#include "fl/random.h"


using namespace std;
using namespace fl;


// class SVM ---------------------------------------------------------------

uint32_t SVM::serializeVersion = 0;

SVM::SVM ()
{
  model = 0;
  metric = 0;

  parameters.svm_type     = C_SVC;
  parameters.kernel_type  = RBF;
  parameters.degree       = 3;
  parameters.gamma        = NAN;  // see run() for data-specific choice
  parameters.coef0        = 0;
  parameters.nu           = 0.5;
  parameters.cache_size   = 100;
  parameters.C            = 1;
  parameters.eps          = 1e-3;
  parameters.p            = 0.1;
  parameters.shrinking    = 1;
  parameters.probability  = 1;
  parameters.nr_weight    = 0;
  parameters.weight_label = 0;
  parameters.weight       = 0;
}

SVM::~SVM ()
{
  clear ();
  if (metric) delete metric;
}

void
SVM::clear ()
{
  data.clear ();

  if (! model) return;
  if (model->free_sv == 0  &&  model->SV)  // meaning that we allocated the support vectors rather than libSVM
  {
	for (int i = 0; i < model->l; i++) free (model->SV[i]);
  }
  svm_free_and_destroy_model (&model);
}

void
SVM::run (const vector<Vector<float> > & data, const vector<int> & classes)
{
  stop = false;
  clear ();

  // Convert data & classes into libSVM problem
  struct svm_problem problem;
  problem.l = min (data.size (), classes.size ());
  problem.y = (double *)    malloc (problem.l * sizeof (double));
  problem.x = (svm_node **) malloc (problem.l * sizeof (svm_node *));
  for (int i = 0; i < problem.l; i++) problem.y[i] = classes[i];
  if (metric)
  {
	MatrixPacked<float> gram (problem.l);
	for (int i = 0; i < problem.l; i++)
	{
	  for (int j = i; j < problem.l; j++)
	  {
		gram(i,j) = metric->value (data[i], data[j]);
	  }

	  Vector<float> d (problem.l + 1);
	  d[0] = i + 1;  // one-based
	  d.region (1) = gram.column (i);
	  problem.x[i] = vector2node (d);
	}
	this->data = data;
	parameters.kernel_type = PRECOMPUTED;
  }
  else
  {
	for (int i = 0; i < problem.l; i++) problem.x[i] = vector2node (data[i]);
  }

  // Solve it
  if (isnan (parameters.gamma)) parameters.gamma = 1.0 / data[0].rows ();
  model = svm_train (&problem, &parameters);

  // Destroy data (except for support vectors)
  //   Mark support vectors to retain
  vector<int> temp (model->l);
  for (int i = 0; i < model->l; i++)
  {
	temp[i] = model->SV[i][0].index;
	model->SV[i][0].index = -1;
  }
  //   Destroy unmarked data
  for (int i = 0; i < problem.l; i++)
  {
	if (problem.x[i][0].index >= 0) free (problem.x[i]);
  }
  free (problem.x);
  free (problem.y);
  //   Undo marks
  for (int i = 0; i < model->l; i++)
  {
	model->SV[i][0].index = temp[i];
  }
}

int
SVM::classify (const Vector<float> & point)
{
  if (! model) return 0;

  svm_node * x = vector2kernel (point);
  int result = (int) roundp (svm_predict (model, x));
  free (x);
  return result;
  //for (int i = 0; i < model->nr_class; i++) if (model->label[i] == result) return i;
  //throw "unknown class";
}

Vector<float>
SVM::distribution (const Vector<float> & point)
{
  if (! model  ||  model->nr_class == 0)
  {
	Vector<float> result;
	return result;
  }

  svm_node * x = vector2kernel (point);
  Vector<double> probabilities (classCount ());
  svm_predict_probability (model, x, &probabilities[0]);
  free (x);
  return probabilities;
}

int
SVM::classCount ()
{
  if (! model) return 0;
  return model->nr_class;
}

Vector<float>
SVM::representative (int group)
{
  if (! model) throw "No SVM model built yet.";
  if (group < 0  ||  group >= model->nr_class) throw "group number out of range";

  int lo = 0;
  for (int i = 0; i < group; i++) lo += model->nSV[i];
  int hi = lo + model->nSV[group];

  if (metric)
  {
	for (int i = lo; i < hi; i++)
	{
	  int index = (int) roundp (model->SV[i]->value);
	  svm_node * x = vector2kernel (data[index]);
	  if (svm_predict (model, x) == group)
	  {
		free (x);
		return data[index];
	  }
	}
	return data[(int) roundp (model->SV[lo]->value)];
  }
  else
  {
	for (int i = lo; i < hi; i++)
	{
	  if (svm_predict (model, model->SV[i]) == group) return node2vector (model->SV[i]);
	}
	return node2vector (model->SV[lo]);
  }
}

svm_node *
SVM::vector2kernel (const Vector<float> & datum)
{
  if (metric)
  {
	int count = data.size ();
	Vector<float> p (count + 1);
	p[0] = 0;
	for (int i = 1; i <= count; i++) p[i] = metric->value (datum, data[i-1]);
	return vector2node (p);
  }
  else
  {
	return vector2node (datum);
  }
}

svm_node *
SVM::vector2node (const Vector<float> & datum)
{
  int nonzero = datum.norm (0);
  svm_node * nodes = (svm_node *) malloc ((nonzero + 1) * sizeof (svm_node));
  float * begin  = &datum[0];
  float * p      = begin;
  svm_node * end = nodes + nonzero;
  svm_node * n   = nodes;
  while (n < end)
  {
	while (*p == 0) p++;
	n->index = p - begin;
	n->value = *p;
	p++;
	n++;
  }
  n->index = -1;
  return nodes;
}

Vector<float>
SVM::node2vector (const svm_node * node)
{
  int dimension = 0;
  const svm_node * n = node;
  while (n->index >= 0) n++;
  n--;
  Vector<float> result (n->index + 1);
  result.clear ();
  n = node;
  while (n->index >= 0)
  {
	result[n->index] = n->value;
	n++;
  }
  return result;
}

void
SVM::serialize (Archive & archive, uint32_t version)
{
  archive & *((ClusterMethod *) this);

  if (archive.in) clear ();
  if (! model)
  {
	model = (svm_model *) malloc (sizeof (svm_model));  // use malloc(), because libsvm uses it internally, so necessary to balance with internal free().
	memset (model, 0, sizeof (svm_model));
	model->param = parameters;  // be careful with weight and weight_label fields!
  }

  archive & model->param.svm_type;
  archive & model->param.kernel_type;
  archive & model->param.degree;
  archive & model->param.gamma;
  archive & model->param.coef0;
  archive & model->nr_class;
  archive & model->l;

  int kk2 = model->nr_class * (model->nr_class - 1) / 2;

  int count = model->rho ? kk2 : 0;
  archive & count;
  if (count  &&  ! model->rho) model->rho = (double *) malloc (count * sizeof (double));
  for (int i = 0; i < count; i++) archive & model->rho[i];

  count = model->label ? model->nr_class : 0;
  archive & count;
  if (count  &&  ! model->label) model->label = (int *) malloc (count * sizeof (int));
  for (int i = 0; i < count; i++) archive & model->label[i];

  count = model->probA ? kk2 : 0;
  archive & count;
  if (count  &&  ! model->probA) model->probA = (double *) malloc (count * sizeof (double));
  for (int i = 0; i < count; i++) archive & model->probA[i];

  count = model->probB ? kk2 : 0;
  archive & count;
  if (count  &&  ! model->probB) model->probB = (double *) malloc (count * sizeof (double));
  for (int i = 0; i < count; i++) archive & model->probB[i];

  count = model->nSV ? model->nr_class : 0;
  archive & count;
  if (count  &&  ! model->nSV) model->nSV = (int *) malloc (count * sizeof (int));
  for (int i = 0; i < count; i++) archive & model->nSV[i];

  int m = model->nr_class - 1;
  if (m  &&  model->l  &&  ! model->sv_coef)
  {
	model->sv_coef = (double **) malloc (m * sizeof (double *));
	for (int i = 0; i < m; i++) model->sv_coef[i] = (double *) malloc (model->l * sizeof (double));
  }
  for (int i = 0; i < m; i++)
  {
	for (int j = 0; j < model->l; j++)
	{
	  archive & model->sv_coef[i][j];
	}
  }

  if (model->l  &&  ! model->SV)
  {
	count = model->l * sizeof (svm_node *);
	model->SV = (svm_node **) malloc (count);
	memset (model->SV, 0, count);
  }
  for (int i = 0; i < model->l; i++)
  {
	count = 0;
	if (model->SV[i])
	{
	  count = 1;
	  if (model->param.kernel_type != PRECOMPUTED) while (model->SV[i][count].index >= 0) count++;
	}
	archive & count;
	if (! model->SV[i]) model->SV[i] = (svm_node *) malloc ((count + 1) * sizeof (svm_node));
	for (int j = 0; j < count; j++)
	{
	  archive & model->SV[i][j].index;
	  archive & model->SV[i][j].value;
	}
	if (archive.in) model->SV[i][count].index = -1;
  }

  model->free_sv = 0;

  archive & metric;
  archive & data;
}
