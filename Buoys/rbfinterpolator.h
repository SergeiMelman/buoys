//- https://people.sc.fsu.edu/~jburkardt/cpp_src/rbf_interp_2d/rbf_interp_2d.html
//- Интерполяция методом RBF (radial basis functions)
//- Радиальные базисные функции (Radial basis functions) (RBF)
//- представляют собой набор методов жесткой интерполяции; это означает,
//- что поверхность должна проходить через каждое измеренное опорное значение.
//- Методы РБФ – это специальный случай сплайнов.

//- Внимание!!!
//- есть мысль, что радиальную функцию можно и нужно рассчитывать
//- не в нуле а в среднем значении по точкам.
//- иначе на поле констант наблюдается притяжение к нулю. И это плохо ибо
//- все известные данные бывают сильно далеко от нуля а их все равно тянет к нулю

#ifndef RBFINTERPOLATOR_H
#define RBFINTERPOLATOR_H

#include <Eigen/Dense>

template <typename TYPE, int Pd = 1, int Fd = 1>
class RBFInterpolator
{
public:
	typedef Eigen::Matrix<TYPE, Pd, 1> Point;
	typedef Eigen::Matrix<TYPE, Fd, 1> Data;
private:
	typedef Eigen::Matrix<Point, Eigen::Dynamic, 1> Points;
	typedef Eigen::Matrix<Data, Eigen::Dynamic, 1> Datas;
	// тип свободных членов
	typedef Eigen::Matrix<TYPE, Eigen::Dynamic, 1> Ws;
	// тип матрицы СЛАУ
	typedef Eigen::Matrix<TYPE, Eigen::Dynamic, Eigen::Dynamic> A;

public:
	void add(const Point& p, const Data& d, bool needCalculate = true)
	{
		const int size = data.rows();
		points.conservativeResize(size + 1, Eigen::NoChange);
		data.conservativeResize(size + 1, Eigen::NoChange);
		points(size) = p;
		data(size) = d;

		if(needCalculate)
		{
			calculate();
		}
	}
	void calculate()
	{
		const int size = data.rows();
		// основная матрица СЛАУ
		A a(size, size);
		// заполнить основную матрицу СЛАУ
		for(int n = 0; n < size; ++n )
		{
			for(int m = 0; m < size; ++m)
			{
				a(m, n) = radialBaseFunc((points[m] - points[n]).norm());
			}
		}
		// рассчитать коэф-ы весов вкладов в результирующую сумму
		for(int i = 0; i < Fd; ++i)
		{
			// эта хрень делает вектор правых элементов для СЛАУ из i-ых
			// позиций в данных.
			typedef Eigen::Map<Ws, 0, Eigen::InnerStride<Fd> > MapMat;
			const MapMat& d1 = MapMat(&(data[0][i]), size);
			w[i] = a.fullPivLu().solve(d1); // решение СЛАУ
		}
	}
	Data operator()(const Point& p) const
	{
		Data rez = Data::Zero();
		for(int i = w[0].rows() - 1; i >= 0; --i)
		{
			const TYPE d = radialBaseFunc((p - points[i]).norm());
			for(int n = 0; n < Fd; ++n)
			{
				rez[n] += w[n][i] * d;
			}
		}
		return rez;
	}

private:
	TYPE radialBaseFunc(TYPE r) const
	{
		return r;// упращенная (multiquadric) Функция мультиквадриков r0 = 0;
		//const TYPE r0 = 0; // параметр (подбирается)
		//return sqrt (r*r + r0*r0); // (multiquadric) Функция мультиквадриков
		//return 1.0 / sqrt (r*r + r0*r0); // (inverse multiquadric) Функция Обратные мультиквадрики
		//return r*r * log(r/r0); // (thin plate spline) Плоский сплайн
		//return exp( -0.5*r*r/(r0*r0)); // (gaussian)
	}
private:
	Points points; // точки в кот известны данные
	Datas data; // значения в точках // матр свободных членов СЛАУ
	Ws w[Fd];
};


#endif
