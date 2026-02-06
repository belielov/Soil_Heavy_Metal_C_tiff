#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include "json/json.hpp"
#include <xgboost/c_api.h>
#include "gdal.h"       
#include "cpl_conv.h" 

using json = nlohmann::json;

#define SAFE_XGB(call) { \
    int err = (call); \
    if (err != 0) { \
        fprintf(stderr, "XGBoost Error: %s\n", XGBGetLastError()); \
        exit(1); \
    } \
}

struct Scaler {
    std::vector<double> mean;
    std::vector<double> scale;
};

// 矩阵特征处理逻辑：支持多行像素同时处理
void process_block_features(const std::vector<float>& input_bands, int nPixels, const Scaler& sc, std::vector<float>& out_features) {
    int nFeatures = 18; 
    out_features.assign(nPixels * nFeatures, 0.0f);

    for (int j = 0; j < nPixels; j++) {
        std::vector<float> b;
        for (int i = 0; i < 11; i++) {
            b.push_back(input_bands[i * nPixels + j] / 10000.0f);
        }

        float eps = 1e-10f;
        std::vector<float> f = b;
        f.push_back((b[10] - b[3]) / (b[10] + b[3] + eps));      
        f.push_back((b[2] - b[10]) / (b[2] + b[10] + eps));     
        float L = 0.5f;
        f.push_back((1.0f + L) * (b[10] - b[3]) / (b[10] + b[3] + L + eps)); 
        f.push_back((b[3] - b[2]) / (b[3] + b[2] + eps));       
        f.push_back(b[8] / (b[9] + eps));                       
        f.push_back(b[4] / (b[5] + eps));                       
        f.push_back(b[8] - b[9]);                               

        for (int k = 0; k < nFeatures; k++) {
            out_features[j * nFeatures + k] = (f[k] - (float)sc.mean[k]) / (float)sc.scale[k];
        }
    }
}

int main(int argc, char* argv[]) {
    CPLSetConfigOption("GTIFF_SRS_SOURCE", "GEOKEYS");
    CPLSetConfigOption("GDAL_DATA", "../gdal-data");
    CPLSetConfigOption("PROJ_LIB", "../gdal-data");
    GDALAllRegister();

    try {
        BoosterHandle booster;
        SAFE_XGB(XGBoosterCreate(NULL, 0, &booster));
        SAFE_XGB(XGBoosterLoadModel(booster, "../model/v5_xgb_model.json"));
        SAFE_XGB(XGBoosterSetParam(booster, "nthread", "0"));

        std::ifstream sc_file("../model/scaler_params.json");
        json j_sc; sc_file >> j_sc;
        Scaler sc = { j_sc["mean"].get<std::vector<double>>(), j_sc["scale"].get<std::vector<double>>() };

        GDALDatasetH hSrcDS = GDALOpen("../input.tif", GA_ReadOnly);
        int nXSize = GDALGetRasterXSize(hSrcDS);
        int nYSize = GDALGetRasterYSize(hSrcDS);

        GDALDriverH hDriver = GDALGetDriverByName("GTiff");
        GDALDatasetH hDstDS = GDALCreate(hDriver, "../output_prediction.tif", nXSize, nYSize, 1, GDT_Float32, NULL);
        
        GDALSetProjection(hDstDS, GDALGetProjectionRef(hSrcDS));
        double adfGeoTransform[6];
        if (GDALGetGeoTransform(hSrcDS, adfGeoTransform) == CE_None) GDALSetGeoTransform(hDstDS, adfGeoTransform);

        // --- 性能优化：增大分块大小 ---
        int nBlockRows = 300; // 一次处理 300 行
        std::vector<float> block_bands(nXSize * nBlockRows * 11);
        std::vector<float> feature_matrix;
        std::vector<float> result_block(nXSize * nBlockRows);

        for (int i = 0; i < nYSize; i += nBlockRows) {
            // 计算当前实际能读取的行数（防止最后一块越界）
            int nCurrentRows = (i + nBlockRows > nYSize) ? (nYSize - i) : nBlockRows;
            int nPixelsInBlock = nXSize * nCurrentRows;

            // 1. 读取数据并处理返回值以消除警告
            for (int b = 0; b < 11; b++) {
                GDALRasterBandH hBand = GDALGetRasterBand(hSrcDS, b + 1);
                CPLErr eErr = GDALRasterIO(hBand, GF_Read, 0, i, nXSize, nCurrentRows, 
                                           &block_bands[b * nPixelsInBlock], nXSize, nCurrentRows, 
                                           GDT_Float32, 0, 0);
                if (eErr != CE_None) fprintf(stderr, "Error reading block at line %d\n", i); // 
            }

            // 2. 特征工程 (处理整个 block)
            process_block_features(block_bands, nPixelsInBlock, sc, feature_matrix);

            // 3. 批量推理
            DMatrixHandle dmatrix;
            SAFE_XGB(XGDMatrixCreateFromMat(feature_matrix.data(), nPixelsInBlock, 18, -1.0, &dmatrix));
            
            bst_ulong out_len;
            const float* out_result;
            SAFE_XGB(XGBoosterPredict(booster, dmatrix, 0, 0, 0, &out_len, &out_result));
            
            for (int j = 0; j < nPixelsInBlock; j++) {
                result_block[j] = std::exp(out_result[j]);
            }

            // 4. 写入结果并处理返回值以消除警告
            GDALRasterBandH hDstBand = GDALGetRasterBand(hDstDS, 1);
            CPLErr wErr = GDALRasterIO(hDstBand, GF_Write, 0, i, nXSize, nCurrentRows, 
                                       result_block.data(), nXSize, nCurrentRows, 
                                       GDT_Float32, 0, 0);
            if (wErr != CE_None) fprintf(stderr, "Error writing block at line %d\n", i); // 

            XGDMatrixFree(dmatrix);
            //当进度达到 10% 的整数倍时打印
            int current_progress = (i * 100 / nYSize);
            if (i == 0 || (i > 0 && current_progress % 10 == 0 && (i - nBlockRows) * 100 / nYSize < current_progress)) {
                std::cout << "Progress: " << current_progress << "%" << std::endl;
            }
        }

        GDALClose(hSrcDS);
        GDALClose(hDstDS);
        XGBoosterFree(booster);
        std::cout << "\n[Success] Processing completed!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
    }
    return 0;
}