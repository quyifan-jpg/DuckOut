#!/bin/bash

# 使用可用的RDMA传输方式
export UCX_TLS=rc,rc_v,rc_verbs
export UCX_NET_DEVICES=mlx4_0:1

# RDMA配置参数
export UCX_IB_RX_QUEUE_LEN=256
export UCX_IB_TX_QUEUE_LEN=256
export UCX_IB_REG_METHODS=odp

# 启用RDMA连接管理器
export UCX_USE_RDMA_CM=yes

# 禁用TCP传输
export UCX_TCP_CM_ENABLE=n

# 启用详细日志以便调试
export UCX_LOG_LEVEL=info

echo "RDMA environment variables have been set:"
env | grep UCX_ 