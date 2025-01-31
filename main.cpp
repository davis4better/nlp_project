#include "Defines.h"
// 这是一个C++函数，它接受三个复数（result1、result2和result3），并检查它们的实部和虚部是否在预期值的一定范围内。
// 如果差异小于容限值，则打印成功消息，否则打印失败消息。

// 验证函数，采用相对误差，不要修改
inline void correntess(ComplexType result1, ComplexType result2, ComplexType result3)
{
	double re_diff, im_diff;

	re_diff = fabs(result1.real() - -264241151.454552);
	im_diff = fabs(result1.imag() - 1321205770.975190);
	re_diff += fabs(result2.real() - -137405397.758745);
	im_diff += fabs(result2.imag() - 961837795.884157);
	re_diff += fabs(result3.real() - -83783779.241634);
	im_diff += fabs(result3.imag() - 754054017.424472);
	printf("%f,%f\n", re_diff, im_diff);
	printf("%f, %f\n", fabs(-264241151.454552 + -137405397.758745 + -83783779.241634) * 1e-6, fabs(1321205770.975190 + 961837795.884157 + 754054017.424472) * 1e-6);

	if (re_diff < fabs(result1.real() + result2.real() + result3.real()) * 1e-6 && im_diff < fabs(result1.imag() + result2.imag() + result3.imag()) * 1e-6)
	{
		printf("\n!!!! SUCCESS - !!!! Correctness test passed :-D :-D\n\n");
	}
	else
	{
		printf("\n!!!! FAILURE - Correctness test failed :-( :-(  \n");
	}
}

int main(int argc, char **argv)
{
	// 定义默认值，不要修改
	int number_bands = 0, nvband = 0, ncouls = 0, nodes_per_group = 0;
	int npes = 1;
	if (argc == 1)
	{
		number_bands = 512;
		nvband = 2;
		ncouls = 32768;
		nodes_per_group = 20;
	}
	else if (argc == 5)
	{
		number_bands = atoi(argv[1]);
		nvband = atoi(argv[2]);
		ncouls = atoi(argv[3]);
		nodes_per_group = atoi(argv[4]);
	}
	else
	{
		std::cout << "The correct form of input is : " << endl;
		std::cout << " ./main.exe <number_bands> <number_valence_bands> "
					 "<number_plane_waves> <nodes_per_mpi_group> "
				  << endl;
		exit(0);
	}
	int ngpown = ncouls / (nodes_per_group * npes);

	// Constants that will be used later
	// 定义常量，不要修改
	const DataType e_lk = 10;
	const DataType dw = 1;
	const DataType to1 = 1e-6;
	const DataType limittwo = pow(0.5, 2);
	const DataType e_n1kq = 6.0;

	// Using time point and system_clock
	time_point<system_clock> start, end, k_start, k_end;
	start = system_clock::now();
	double elapsedKernelTimer;

	// Printing out the params passed.
	std::cout << "Sizeof(ComplexType = "
			  << sizeof(ComplexType) << " bytes" << std::endl;
	std::cout << "number_bands = " << number_bands << "\t nvband = " << nvband
			  << "\t ncouls = " << ncouls
			  << "\t nodes_per_group  = " << nodes_per_group
			  << "\t ngpown = " << ngpown << "\t nend = " << nend
			  << "\t nstart = " << nstart << endl;

	size_t memFootPrint = 0.00;

	// ALLOCATE statements .
	// 声明数组，在‘arrayMD.h’中
	ARRAY1D achtemp(nend - nstart);
	memFootPrint += (nend - nstart) * sizeof(ComplexType);

	ARRAY2D aqsmtemp(number_bands, ncouls);
	ARRAY2D aqsntemp(number_bands, ncouls);
	memFootPrint += 2 * (number_bands * ncouls) * sizeof(ComplexType);

	ARRAY2D I_eps_array(ngpown, ncouls);
	ARRAY2D wtilde_array(ngpown, ncouls);
	memFootPrint += 2 * (ngpown * ncouls) * sizeof(ComplexType);

	ARRAY1D_DataType vcoul(ncouls);
	memFootPrint += ncouls * sizeof(DataType);

	ARRAY1D_int inv_igp_index(ngpown);
	ARRAY1D_int indinv(ncouls + 1);
	memFootPrint += ngpown * sizeof(int);
	memFootPrint += (ncouls + 1) * sizeof(int);

	ARRAY1D_DataType wx_array(nend - nstart);
	memFootPrint += 3 * (nend - nstart) * sizeof(DataType);

	// Print Memory Foot print
	cout << "Memory Foot Print = " << memFootPrint / pow(1024, 3) << " GBs"
		 << endl;
	// 测试的时候不一定是常数优化，但可以用0.5来算
	ComplexType expr(.5, .5);
	for (int i = 0; i < number_bands; i++)
		for (int j = 0; j < ncouls; j++)
		{
			aqsmtemp(i, j) = expr;
			aqsntemp(i, j) = expr;
		}

	for (int i = 0; i < ngpown; i++)
		for (int j = 0; j < ncouls; j++)
		{
			I_eps_array(i, j) = expr;
			wtilde_array(i, j) = expr;
		}

	for (int i = 0; i < ncouls; i++)
		vcoul(i) = 1.0;

	for (int ig = 0; ig < ngpown; ++ig)
		inv_igp_index(ig) = (ig + 1) * ncouls / ngpown;

	for (int ig = 0; ig < ncouls; ++ig)
		indinv(ig) = ig;
	indinv(ncouls) = ncouls - 1;

	for (int iw = nstart; iw < nend; ++iw)
	{
		wx_array(iw) = e_lk - e_n1kq + dw * ((iw + 1) - 2);
		if (wx_array(iw) < to1)
			wx_array(iw) = to1;
	}

	k_start = system_clock::now();
	noflagOCC_solver(number_bands, ngpown, ncouls, inv_igp_index, indinv,
					 wx_array, wtilde_array, aqsmtemp, aqsntemp, I_eps_array,
					 vcoul, achtemp);

	k_end = system_clock::now();
	duration<double> elapsed = k_end - k_start;
	elapsedKernelTimer = elapsed.count();

	// Check for correctness
	correntess(achtemp(0), achtemp(1), achtemp(2));

	printf("\n Final achtemp\n");
	ComplexType_print(achtemp(0));
	ComplexType_print(achtemp(1));
	ComplexType_print(achtemp(2));

	end = system_clock::now();
	elapsed = end - start;

	cout << "********** Kernel Time Taken **********= " << elapsedKernelTimer
		 << " secs" << endl;
	cout << "********** Total Time Taken **********= " << elapsed.count()
		 << " secs" << endl;

	return 0;
}

void noflagOCC_solver(size_t number_bands, size_t ngpown, size_t ncouls,
					  ARRAY1D_int &inv_igp_index, ARRAY1D_int &indinv,
					  ARRAY1D_DataType &wx_array, ARRAY2D &wtilde_array,
					  ARRAY2D &aqsmtemp, ARRAY2D &aqsntemp,
					  ARRAY2D &I_eps_array, ARRAY1D_DataType &vcoul,
					  ARRAY1D &achtemp)
{
	time_point<system_clock> start, end;
	start = system_clock::now();

	DataType ach_re0 = 0.00, ach_re1 = 0.00, ach_re2 = 0.00, ach_im0 = 0.00,
			 ach_im1 = 0.00, ach_im2 = 0.00;

	for (int n1 = 0; n1 < number_bands; ++n1)
	{
		for (int ig = 0; ig < ncouls; ++ig)
		{
			for (int my_igp = 0; my_igp < ngpown; ++my_igp)
			{
				int indigp = inv_igp_index(my_igp);
				int igp = indinv(indigp);
				DataType achtemp_re_loc[nend - nstart], achtemp_im_loc[nend - nstart];
				// 初始化复数
				for (int iw = nstart; iw < nend; ++iw)
				{
					achtemp_re_loc[iw] = 0.00;
					achtemp_im_loc[iw] = 0.00;
				}
				// 引入杂化函数
				ComplexType sch_store1 =
				ComplexType_conj(aqsmtemp(n1, igp)) * aqsntemp(n1, igp) * 0.5 *vcoul(igp) * wtilde_array(my_igp, igp);

				for (int iw = nstart; iw < nend; ++iw)
				{
					//差值
					ComplexType wdiff =
						wx_array(iw) - wtilde_array(my_igp, ig);
					ComplexType delw =
						ComplexType_conj(wdiff) *
						(1 / (wdiff * ComplexType_conj(wdiff)).real());
					ComplexType sch_array =
						delw * I_eps_array(my_igp, ig) * sch_store1;

					achtemp_re_loc[iw] += (sch_array).real();
					achtemp_im_loc[iw] += (sch_array).imag();
				}
				ach_re0 += achtemp_re_loc[0];
				ach_re1 += achtemp_re_loc[1];
				ach_re2 += achtemp_re_loc[2];
				ach_im0 += achtemp_im_loc[0];
				ach_im1 += achtemp_im_loc[1];
				ach_im2 += achtemp_im_loc[2];
			}
		}
	}

	achtemp(0) = ComplexType(ach_re0, ach_im0);
	achtemp(1) = ComplexType(ach_re1, ach_im1);
	achtemp(2) = ComplexType(ach_re2, ach_im2);
}
