#include <QVector>
#include <QtAlgorithms>
#include <math.h>

template <typename T>
class ReversedCyclicBuffer {
public:
	inline ReversedCyclicBuffer(int size = 0) : buffer(size), position(0)
	{
	}
	void init(int size)
	{
		buffer.clear();
		buffer.resize(size);
	}
	void fill(const T& value)
	{
		buffer.fill(value);
	}
	void push(const T& value)
	{
		if (!position)
			position = buffer.size();
		buffer[--position] = value;
	}
	T &operator[](int i)
	{
		if (i < 0)
			i = 0;
		else if (i >= buffer.size())
			i = buffer.size() - 1;
		i += position;
		if (i >= buffer.size())
			i -= buffer.size();
		return buffer[i];
	}
	int size()
	{
		return buffer.size();
	}
private:
	QVector<T> buffer;
	unsigned int position;
};


//
// DigitalFilter_u16 is superclass for digital filters for 16-bit samples
// DigitalFilter_u16 itself returns input samples without filtering
//

class DigitalFilter_u16 {
public:
	DigitalFilter_u16() { }
	virtual void init(unsigned short /* value */) { }
	virtual unsigned short filter(unsigned short value) { return value; }
};


//
// MovingAverageFilter_u16 is moving average of 2^logWindowSize last samples
//

class MovingAverageFilter_u16 : public DigitalFilter_u16 {
public:
	MovingAverageFilter_u16(unsigned int logWindowSize) : sum(0) { x.init(1 << logWindowSize); this->logWindowSize = logWindowSize; }
	void init(unsigned short value)
	{
		sum = value << logWindowSize;
		x.fill(value);
	}
	unsigned short filter(unsigned short value)
	{
		sum += value - x[x.size() - 1];
		x.push(value);
		return sum >> logWindowSize;
	}
private:
	ReversedCyclicBuffer<unsigned short> x;
	unsigned int sum;
	unsigned int logWindowSize;
};


//
// LowPassRCFilter_u16 is digital low-pass RC filter with tau = 2^logK * delta_t
//

class LowPassRCFilter_u16 : public DigitalFilter_u16 {
public:
	LowPassRCFilter_u16(unsigned int logK) : ky(0) { this->logK = logK; }
	void init(unsigned short value)
	{
		ky = value << logK;
	}
	unsigned short filter(unsigned short value)
	{						// y(n) = y(n-1) + (x(n) - y(n-1)) / k, k = 2^noiseBits
		ky += value - (ky >> logK);		// k * y(n) = k * y(n-1) + x(n) - k * y(n-1) / k
		return ky >> logK;			// y(n) = k * y(n) / k
	}
private:
	unsigned int ky;			// store ky instead of y to prevent rounding error accumulation
	unsigned int logK;
};


//
// MedianFilter_u16 is median filter for windowSize last values
//

class MedianFilter_u16 : public DigitalFilter_u16 {
public:
	MedianFilter_u16(int windowSize)
	{
		this->windowSize = windowSize;
		x.init(windowSize);
		x_ordered.resize(windowSize);
	}
	void init(unsigned short value)
	{
		x_ordered.fill(value);
		x.fill(value);
	}
	unsigned short filter(unsigned short value)
	{
		x_ordered.remove(qUpperBound(x_ordered.begin(), x_ordered.end(), x[x.size() - 1]) - x_ordered.begin() - 1);
		x_ordered.insert(qUpperBound(x_ordered.begin(), x_ordered.end(), value) - x_ordered.begin(), value);
		x.push(value);
		return x_ordered[x_ordered.size() >> 1];
	}
private:
	ReversedCyclicBuffer<unsigned short> x;
	QVector<unsigned short> x_ordered;
	int windowSize;
};


//
// MedianLowPassRCFilter_u16 is combination of median filter and low-pass RC filter with variable tau
//

class MedianLowPassRCFilter_u16 : public DigitalFilter_u16 {
public:
	static const int windowSize = 8;
	static const int minimalLogK = 2;
	static const int maximalLogK = 8;
	static const int smallStepsToDecreaseLogK = 3;

	MedianLowPassRCFilter_u16(int medianWindowSize, unsigned int noiseBits) : ky(0), logK(minimalLogK), smallStepsWithEqualLogK(0), dif_sum(0), medianFilter(medianWindowSize)
	{
		this->noiseBits = noiseBits;
		dif.init(windowSize);
	}
	void init(unsigned short value)
	{
		ky = value << maximalLogK;
		medianFilter.init(value);
	}
	unsigned short filter(unsigned short value)
	{
		unsigned int u = medianFilter.filter(value) << maximalLogK;
		int dif_cur = (value << maximalLogK) - ky;			// maxK * (x(n) - y(n-1))
		if ((dif_cur > 0 ? dif_cur : -dif_cur) >> (maximalLogK + noiseBits)) {
			if ((u > ky ? u - ky : ky - u) >> (maximalLogK + noiseBits))
				ky = u;
			logK = minimalLogK;
			smallStepsWithEqualLogK = 0;
		}
		else {
			if (dif_sum <= 0 && dif_cur >= 0 || dif_sum >= 0 && dif_cur <= 0) {
				if (logK < maximalLogK) {
					smallStepsWithEqualLogK = 0;
					++logK;
				}
			}
			else if (smallStepsWithEqualLogK == smallStepsToDecreaseLogK) {
				if (logK > minimalLogK) {
					smallStepsWithEqualLogK = 0;
					--logK;
				}
			}
			if (smallStepsWithEqualLogK < smallStepsToDecreaseLogK)
				++smallStepsWithEqualLogK;
										// y(n) = y(n-1) + (x(n) - y(n-1)) / k, k = 2^logK
			ky += dif_cur >> logK;					// maxK * y(n) = maxK * y(n-1) + maxK * (x(n) - y(n-1)) / k
			dif_sum += dif_cur - dif[dif.size() - 1];
			dif.push(dif_cur);
		}
		return ky >> maximalLogK;
	}
private:
	unsigned int noiseBits;
	unsigned int ky;
	unsigned int logK;
	unsigned int smallStepsWithEqualLogK;
	int dif_sum;
	ReversedCyclicBuffer<int> dif;
	MedianFilter_u16 medianFilter;
};


class HysteresisFilter_double {
public:
	HysteresisFilter_double(double discreteness = 0) : m_discreteness(discreteness), y(0) { }
	double discreteness() const			{ return m_discreteness; }
	void setDiscreteness(double discreteness)	{ m_discreteness = discreteness; }
	double filter(double value)
	{
		if (m_discreteness <= 0) {
			y = value;
		}
		else if (fabs(y - value) > m_discreteness) {
			y = floor(value / m_discreteness + 0.5) * m_discreteness;
		}
		return y;
	}
private:
	double m_discreteness;
	double y;
};


class HysteresisFilter_u16 : public DigitalFilter_u16 {
public:
	HysteresisFilter_u16(unsigned short noiseBits) { this->noiseBits = noiseBits; increment = 1 << (noiseBits - 1); mask = ((unsigned short)-1) >> noiseBits << noiseBits; }
	void init(unsigned short value)
	{
		y = value;
	}
	unsigned short filter(unsigned short value)
	{
		if ((y > value ? y - value : value - y) >> noiseBits) {
			y = (value + increment) & mask;
		}
		return y;
	}
private:
	unsigned short noiseBits;
	unsigned short increment;
	unsigned short mask;
	unsigned short y;
};


class CombinedFilter_u16 : public DigitalFilter_u16 {
public:
	CombinedFilter_u16(DigitalFilter_u16 *y, DigitalFilter_u16 *u) { this->y = y; this->u = u; }
	~CombinedFilter_u16() { delete y; delete u; }
	void init(unsigned short value)
	{
		y->init(value);
		u->init(value);
	}
	unsigned short filter(unsigned short value)
	{
		return y->filter(u->filter(value));
	}
private:
	DigitalFilter_u16 *y, *u;
};
