# -*- coding: utf-8 -*-
"""
Created on Thu Apr  9 15:06:38 2020

@author: b6462
"""

import numpy as np
import matplotlib.pyplot as plt
import cmath

RMSE_val_p_all = 0
RMSE_val_v_all = 0
plot_len = 50
cycle = 200
rel = 0.001

for j in range(1, 1 + cycle):
    org = np.linspace(1,plot_len,plot_len) #初值设定时间线

    
    R_ek = 0.8**2
    Q_k = (rel**2)*R_ek/(1-rel**2) #wk协方差
    R_k = Q_k + R_ek #ek协方差

    v_spread = np.random.normal(5, 5, size=(org.shape[0]))
    v = v_spread[0]
    p_spread = np.random.normal(10,100,size=(org.shape[0]))
    pos_0 = p_spread[0]

    t = np.linspace(1,plot_len,plot_len) #主时间线

    pos_real = pos_0 + v * t #真实值 完成对每一步的变换矩阵

    wk = np.random.normal(0,Q_k,size=(t.shape[0])) #wk 不相关正态分布噪声(均值,标准差,存储)
    ek = np.random.normal(0,R_k,size=(t.shape[0])) #ek 不相关正态分布噪声
    pos_req = pos_real + wk + ek#测量值
    """
    plt.plot(t,pos_real,label='real')
    plt.plot(t,pos_req,label='acquired')
    """
    
    predicts = [pos_req[0]] #滤波输出数组,初次迭代只有测量值
    pos_predict = predicts[0]
    
    C_cur = np.mat([1, 0])
    A = np.mat([[1, 1], [0, 1]])
    L = np.mat([[0.5], [1]])
    P_pre = np.mat([[100, 0], [0, 5]])
    P_cur = np.mat([[0, 0], [0, 0]])
    I = np.mat([[1, 0], [0, 1]])
    
    
    x_pre = np.mat([[10], [5]])
    x_cur = np.mat([[0], [0]])
    
    RMSE_val_p = 0
    RMSE_val_v = 0
    
    
    for i in range(1,t.shape[0]):
        
        P_pre = A*P_pre*A.T + L*Q_k*L.T 
        G_cur = (P_pre*C_cur.T) / (C_cur*P_pre*C_cur.T + R_k + Q_k)
        P_cur = (I - G_cur*C_cur)*P_pre
        P_pre = P_cur
        x_pre = A*x_pre
        x_cur = x_pre + G_cur*(pos_req[i] - C_cur*x_pre)
        x_pre = x_cur
        predicts.append(x_cur[0][0]) #只取位置
        RMSE_val_p = RMSE_val_p + (x_cur[0][0] - pos_real[i])**2
        RMSE_val_v = RMSE_val_v + (x_cur[1][0] - v)**2
        
    RMSE_val_p_all = RMSE_val_p_all + RMSE_val_p
    RMSE_val_v_all = RMSE_val_v_all + RMSE_val_v
    
    RMSE_val_p = cmath.sqrt(RMSE_val_p/plot_len)
    RMSE_val_v = cmath.sqrt(RMSE_val_v/plot_len)
    #print("RMSE of position[", j,"]:", RMSE_val_p)
    #print("RMSE of velocitiy[", j,"]:", RMSE_val_v)
    """
    plt.plot(t,predicts,label='kalman-filtered')
    
    plt.legend()
    plt.show()
    """
    
RMSE_val_p_all = cmath.sqrt(RMSE_val_p_all/(plot_len*cycle))
RMSE_val_v_all = cmath.sqrt(RMSE_val_v_all/(plot_len*cycle))
print("\nWhen ρ =", rel, "with", cycle, "samples:")
print("RMSE of ALL position:", RMSE_val_p_all)
print("RMSE of ALL velocitiy:", RMSE_val_v_all)
    
    
    
    
    
    
