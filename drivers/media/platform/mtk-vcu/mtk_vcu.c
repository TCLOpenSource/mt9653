// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/videodev2.h>
#include <linux/dma-mapping.h>

#include "mtk_vcu.h"
#include "mtk_tv_vcu.h"
#include "mtk_vcu_tee_interface.h"

static int mtk_vcu_debug;
module_param(mtk_vcu_debug, int, 0644);
static struct platform_device *vcu_pdev;

#define dprintk(level, fmt, args...)					\
	do {								\
		if (mtk_vcu_debug >= level)				\
			pr_info("[mtk-vcu]%s(),%d: " fmt,		\
			__func__, __LINE__, ##args);			\
	} while (0)

/**
 * struct vcu_ipi_desc - VCU IPI descriptor
 *
 * @handler:    IPI handler
 * @name:       the name of IPI handler
 * @priv:       the private data of IPI handler
 */
struct vcu_ipi_desc {
	ipi_handler_t handler;
	const char *name;
	void *priv;
};

static const struct mtk_tv_vcu_ops *g_tv_vcu_ops[VCU_CODEC_MAX];

struct mtk_vcu {
	struct vcu_ipi_desc ipi_desc[IPI_MAX];
	struct device *dev;

	unsigned long lax_buf_tag;
	unsigned long core_buf_tag;
	unsigned long venc_buf_tag;
	unsigned long vdec_svp_shm_buf_tag;
	struct attribute_group sysfs_group[VCU_CODEC_MAX];

	struct mtk_vcu_reg_param reg_param;
	struct mtk_vcu_hw_info hw_info;
	struct mtk_vcu_irq_param irq_param;
	struct mtk_vcu_dv_info_param dv_info_param;
	struct mtk_vcu_dec_cap_table dec_cap_table;
	struct mtk_vcu_tee_interface tee_interface;
};

static const struct mtk_tv_vcu_ops *get_tv_vcu_ops(enum ipi_id id)
{
	if (id < IPI_VENC_COMMON && id >= IPI_VCU_INIT)
		return g_tv_vcu_ops[VCU_VDEC];
	else if (id < IPI_MDP && id >= IPI_VENC_COMMON)
		return g_tv_vcu_ops[VCU_VENC];
	else
		return NULL;
}

int vcu_ipi_register(struct platform_device *pdev,
		enum ipi_id id, ipi_handler_t handler,
		const char *name, void *priv)
{
	struct mtk_vcu *vcu = platform_get_drvdata(pdev);
	struct vcu_ipi_desc *ipi_desc;

	if (vcu == NULL) {
		dprintk(0, "vcu device in not ready\n");
		return -EPROBE_DEFER;
	}

	if (id >= IPI_VCU_INIT && id < IPI_MAX && handler != NULL) {
		ipi_desc = vcu->ipi_desc;
		ipi_desc[id].name = name;
		ipi_desc[id].handler = handler;
		ipi_desc[id].priv = priv;
		return 0;
	}

	dprintk(0, "register vcu ipi id %d with invalid arguments\n", id);
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(vcu_ipi_register);

static unsigned long vcu_get_buf_tag(struct mtk_vcu *vcu,
		enum mtk_vcu_buffer_type type)
{
	dprintk(2, "get buf tag for type %d\n", type);

	switch (type) {
	case BUFFER_TYPE_LAX:
		return vcu->lax_buf_tag;
	case BUFFER_TYPE_CORE:
		return vcu->core_buf_tag;
	case BUFFER_TYPE_VENC:
		return vcu->venc_buf_tag;
	case BUFFER_TYPE_SVP_SHM:
		return vcu->vdec_svp_shm_buf_tag;
	}

	return 0;
}

static enum dma_data_direction vcu_get_buf_data_direction(
	enum mtk_vcu_buffer_sync_dir dir)
{
	switch (dir) {
	case BUFFER_SYNC_BIDIRECTION:
		return DMA_BIDIRECTIONAL;
	case BUFFER_SYNC_TO_DEVICE:
		return DMA_TO_DEVICE;
	case BUFFER_SYNC_FROM_DEVICE:
		return DMA_FROM_DEVICE;
	default:
		return DMA_NONE;
	}
}

static struct sg_table *vcu_create_buf_sg_table(void *context,
	struct mtk_vcu_buffer_info *buf_info)
{
	struct mtk_vcu *vcu = context;
	struct device *dev = vcu->dev;
	struct sg_table *sgt;
	int ret;

	sgt = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!sgt) {
		dprintk(0, "alloc sgt fail va %lx iova %llx size 0x%zx\n",
			(uintptr_t)buf_info->va, (u64)buf_info->iova, buf_info->size);
		return NULL;
	}

	ret = dma_get_sgtable_attrs(dev, sgt, buf_info->va, buf_info->iova, buf_info->size, 0);
	if (ret) {
		dprintk(0, "get sgt fail ret %d va %lx iova %llx size 0x%zx\n",
			ret, (uintptr_t)buf_info->va, (u64)buf_info->iova, buf_info->size);
		kfree(sgt);
		return NULL;
	}

	return sgt;
}

static void vcu_destroy_buf_sg_table(struct sg_table *sgt)
{
	if (!sgt)
		return;

	sg_free_table(sgt);
	kfree(sgt);
}

static void vcu_free_buf(void *context,
			 enum mtk_vcu_buffer_type type,
			 struct mtk_vcu_buffer_info *buf_info)
{
	struct mtk_vcu *vcu = context;
	struct device *dev = vcu->dev;
	unsigned long buf_tag = vcu_get_buf_tag(vcu, type);

	if (buf_info->va == NULL)
		return;

	dprintk(1, "free buffer size 0x%zx iova %pad tag 0x%lx",
		buf_info->size, &buf_info->iova, buf_tag);
	dma_free_attrs(dev, buf_info->size, buf_info->va,
		buf_info->iova, buf_tag);
	if (buf_info->sg_table)
		vcu_destroy_buf_sg_table(buf_info->sg_table);

	memset(buf_info, 0, sizeof(struct mtk_vcu_buffer_info));
}

static int vcu_alloc_buf(void *context,
			 enum mtk_vcu_buffer_type type,
			 unsigned long flag,
			 size_t size,
			 struct mtk_vcu_buffer_info *buf_info)
{
	struct mtk_vcu *vcu = context;
	struct device *dev = vcu->dev;
	unsigned long buf_tag = vcu_get_buf_tag(vcu, type);
	unsigned long attrs = buf_tag;

	WARN_ON(buf_info->va != NULL);
	if (size == 0)
		return 0;

	// DMA_ATTR_WRITE_COMBINE for uncached buffer
	if (!(flag & VCU_BUFFER_FLAG_CACHED))
		attrs |= DMA_ATTR_WRITE_COMBINE;
	buf_info->va = dma_alloc_attrs(dev, size, &buf_info->iova, GFP_KERNEL, attrs);
	dprintk(1, "alloc buffer size 0x%zx va %lx iova %pad attrs 0x%lx",
		size, (uintptr_t)buf_info->va, &buf_info->iova, attrs);
	if (buf_info->va) {
		buf_info->size = size;
		buf_info->buf_tag = buf_tag;
	} else {
		buf_info->size = 0;
		dprintk(0, "alloc mem fail for tag 0x%lx", buf_tag);
		return -ENOMEM;
	}

	// cached buffer sg_table
	if (flag & VCU_BUFFER_FLAG_CACHED)
		buf_info->sg_table = vcu_create_buf_sg_table(context, buf_info);
	else
		buf_info->sg_table = NULL;

	return 0;
}

static void vcu_sync_buf(void *context,
			 struct mtk_vcu_buffer_info *buf_info,
			 enum mtk_vcu_buffer_sync_dev sync_dev,
			 enum mtk_vcu_buffer_sync_dir sync_dir)
{
	struct mtk_vcu *vcu = context;
	struct device *dev = vcu->dev;
	struct sg_table *sgt;
	enum dma_data_direction dir = vcu_get_buf_data_direction(sync_dir);

	if (buf_info->va == NULL)
		return;

	if (buf_info->size == 0)
		return;

	if (buf_info->sg_table) {
		sgt = buf_info->sg_table;
	} else {
		sgt = vcu_create_buf_sg_table(context, buf_info);
		if (!sgt) {
			dprintk(0, "create sgt fail va %lx iova %llx size 0x%zx\n",
				(uintptr_t)buf_info->va, (u64)buf_info->iova, buf_info->size);
			goto err_out;
		}
	}

	switch (sync_dev) {
	case BUFFER_SYNC_DEVICE:
		dma_sync_sg_for_device(dev, sgt->sgl, sgt->nents, dir);
		break;
	case BUFFER_SYNC_CPU:
		dma_sync_sg_for_cpu(dev, sgt->sgl, sgt->nents, dir);
		break;
	default:
		dprintk(0, "unsupport buffer sync dev %d\n", sync_dev);
		break;
	}

err_out:
	if (!buf_info->sg_table)
		vcu_destroy_buf_sg_table(sgt);
}

int vcu_ipi_send(struct platform_device *pdev, enum ipi_id id, void *buf,
		unsigned int len)
{
	struct mtk_vcu *vcu = platform_get_drvdata(pdev);
	const struct mtk_tv_vcu_ops *tv_vcu_ops = get_tv_vcu_ops(id);
	struct mtk_tv_vcu_callback cb;

	if (tv_vcu_ops == NULL) {
		dprintk(0, "tv vcu ops is NULL\n");
		return -EINVAL;
	}

	if (!(id >= IPI_VCU_INIT && id < IPI_MAX)) {
		dprintk(0, "invalid ipi_id %d\n", id);
		return -EINVAL;
	}

	memset(&cb, 0, sizeof(struct mtk_tv_vcu_callback));
	cb.alloc_buf = vcu_alloc_buf;
	cb.free_buf = vcu_free_buf;
	cb.sync_buf = vcu_sync_buf;
	cb.context = vcu;
	cb.ipi_handler = vcu->ipi_desc[id].handler;
	cb.ipi_priv = vcu->ipi_desc[id].priv;
	tv_vcu_ops->handler(buf, id, &cb);

	return 0;
}
EXPORT_SYMBOL_GPL(vcu_ipi_send);

unsigned int vcu_get_vdec_hw_capa(struct platform_device *pdev)
{
	return 0;
}
EXPORT_SYMBOL_GPL(vcu_get_vdec_hw_capa);

void *vcu_mapping_dm_addr(struct platform_device *pdev,
		uintptr_t dtcm_dmem_addr)
{
	return (void *)dtcm_dmem_addr;
}
EXPORT_SYMBOL_GPL(vcu_mapping_dm_addr);

struct platform_device *vcu_get_plat_device(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *vcu_node = NULL;
	struct platform_device *vcu_pdev = NULL;

	dev_dbg(&pdev->dev, "[VCU] %s\n", __func__);

	vcu_node = of_parse_phandle(dev->of_node, "mediatek,vcu", 0);
	if (vcu_node == NULL) {
		dev_err(dev, "[VCU] can't get vcu node\n");
		return NULL;
	}

	vcu_pdev = of_find_device_by_node(vcu_node);
	if (WARN_ON(vcu_pdev == NULL) == true) {
		dev_err(dev, "[VCU] vcu pdev failed\n");
		of_node_put(vcu_node);
		return NULL;
	}

	return vcu_pdev;
}
EXPORT_SYMBOL_GPL(vcu_get_plat_device);

int vcu_load_firmware(struct platform_device *pdev)
{
	if (pdev == NULL) {
		dev_err(&pdev->dev, "[VCU] VCU platform device is invalid\n");
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(vcu_load_firmware);

int vcu_compare_version(struct platform_device *pdev,
			const char *expected_version)
{
	return 0;
}
EXPORT_SYMBOL_GPL(vcu_compare_version);

static int mtk_vcu_update_vdec_cap(struct mtk_vcu *vcu)
{
	struct device *dev = vcu->dev;
	struct device_node *np = dev->of_node;
	int ret = 0;
	struct mtk_vcu_dec_cap_table *dec_cap_table = &vcu->dec_cap_table;

	ret = of_property_count_u8_elems(np, "vdec_cap_table");
	if (ret <= 0) {
		dev_info(dev, "get cap table elem fails. ret %d\n", ret);
		return ret;
	}
	dec_cap_table->size = ret;
	dev_info(dev, "size of dec_cap_table is %d\n", dec_cap_table->size);

	dec_cap_table->data = devm_kzalloc(dev, sizeof(*vcu), GFP_KERNEL);
	if (dec_cap_table->data == NULL)
		return -ENOMEM;

	ret = of_property_read_u8_array(np,
					"vdec_cap_table",
					dec_cap_table->data,
					dec_cap_table->size);

	return ret;
}

#define IOMMU_TAG_SHIFT 24
static void mtk_vcu_update_iommu_buf_tag(struct mtk_vcu *vcu)
{
	struct device *dev = vcu->dev;
	struct device_node *iommu_node = NULL;
	struct device_node *np = dev->of_node;
	u32 buf_tag_id = 0;

	vcu->lax_buf_tag = 0;
	vcu->core_buf_tag = 0;
	vcu->venc_buf_tag = 0;
	vcu->vdec_svp_shm_buf_tag = 0;

	iommu_node = of_parse_phandle(np, "iommu_lax_node", 0);
	if (iommu_node) {
		if (of_property_read_u32(iommu_node, "id", &buf_tag_id) == 0) {
			dprintk(0, "lax tag id %d\n", buf_tag_id);
			vcu->lax_buf_tag = buf_tag_id << IOMMU_TAG_SHIFT;
		}
		of_node_put(iommu_node);
	}

	iommu_node = of_parse_phandle(np, "iommu_core_node", 0);
	if (iommu_node) {
		if (of_property_read_u32(iommu_node, "id", &buf_tag_id) == 0) {
			dprintk(0, "core tag id %d\n", buf_tag_id);
			vcu->core_buf_tag = buf_tag_id << IOMMU_TAG_SHIFT;
		}
		of_node_put(iommu_node);
	}

	iommu_node = of_parse_phandle(np, "iommu_venc_node", 0);
	if (iommu_node) {
		if (of_property_read_u32(iommu_node, "id", &buf_tag_id) == 0) {
			dprintk(0, "venc tag id %d\n", buf_tag_id);
			vcu->venc_buf_tag = buf_tag_id << IOMMU_TAG_SHIFT;
		}
		of_node_put(iommu_node);
	}

	iommu_node = of_parse_phandle(np, "iommu_vdec_svp_shm_node", 0);
	if (iommu_node) {
		if (of_property_read_u32(iommu_node, "id", &buf_tag_id) == 0) {
			dprintk(0, "vdec svp shm tag id %d\n", buf_tag_id);
			vcu->vdec_svp_shm_buf_tag = buf_tag_id << IOMMU_TAG_SHIFT;
		}
		of_node_put(iommu_node);
	}

	WARN_ON(vcu->lax_buf_tag == 0);
	WARN_ON(vcu->core_buf_tag == 0);
	WARN_ON(vcu->venc_buf_tag == 0);
	WARN_ON(vcu->vdec_svp_shm_buf_tag == 0);
}

static int mtk_vcu_create_sysfs_attr(struct platform_device *pdev,
				     const struct attribute_group *group)
{
	struct device *dev = &(pdev->dev);
	int ret = 0;

	if (!group)
		return 0;

	/* Create device attribute files */
	dev_info(dev, "Create device attribute group name %s\n", group->name);
	ret = sysfs_create_group(&dev->kobj, group);
	if (ret)
		goto err_out;
	return ret;

err_out:
	/* Remove device attribute files */
	dev_info(dev, "Error Handling: Remove device attribute group\n");
	sysfs_remove_group(&dev->kobj, group);
	return ret;
}

static int mtk_vcu_remove_sysfs_attr(struct platform_device *pdev,
				     const struct attribute_group *group)
{
	struct device *dev = &(pdev->dev);
	int ret = 0;

	if (!group)
		return 0;

	/* Remove device attribute files */
	dev_info(dev, "Remove device attribute group name %s\n", group->name);
	sysfs_remove_group(&dev->kobj, group);

	return ret;
}

static int vcu_parse_dv_profile_level(struct platform_device *pdev, struct mtk_vcu *vcu)
{
	struct mtk_vcu_dv_info_param *dv_info_param = &vcu->dv_info_param;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	int ret = 0;
	int i = 0, entry_count = 0;
	u32 profile = 0, level = 0;

	entry_count = of_property_count_u32_elems(np, "dv_profile_level_matrix");
	if (entry_count <= 0) {
		dev_err(&pdev->dev, "Invalid entry_count:%d\n", entry_count);
		return 0;
	}

	dv_info_param->dv_info_num = entry_count / 2;
	for (i = 0; i < entry_count; i += 2) {
		int index = i / 2;

		ret = of_property_read_u32_index(np, "dv_profile_level_matrix", i, &profile);
		if (ret) {
			dev_err(&pdev->dev, "Invalid dv_profile_level_matrix property\n");
			return 0;
		}

		ret = of_property_read_u32_index(np, "dv_profile_level_matrix", i + 1, &level);
		if (ret) {
			dev_err(&pdev->dev, "Invalid dv_profile_level_matrix property\n");
			return 0;
		}

		dv_info_param->dv_info[index].profile = profile;
		dv_info_param->dv_info[index].highest_level = level;
		dev_info(&pdev->dev, "%s i:%d profile:%d level:%d\n",
			__func__,
			index,
			dv_info_param->dv_info[index].profile,
			dv_info_param->dv_info[index].highest_level);
	}

	return 0;
}

static int vcu_parse_bw_info(struct platform_device *pdev, struct mtk_vcu *vcu)
{
	struct mtk_vcu_hw_info *hw_info = &vcu->hw_info;
	struct device *dev = &pdev->dev;
	int ret = 0;
	int i = 0;
	u32 efficiency = 0;

	ret = of_property_read_u32(dev->of_node, "AV1_COMPRESS_BW_PER_PIXEL",
		&hw_info->bw_per_pixel[AV1_COMPRESS_BW]);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get AV1_COMPRESS_BW_PER_PIXEL. ret %d\n", ret);
		hw_info->bw_per_pixel[AV1_COMPRESS_BW] = 0;
	}

	ret = of_property_read_u32(dev->of_node, "AV1_BW_PER_PIXEL",
		&hw_info->bw_per_pixel[AV1_NO_COMPRESS_BW]);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get AV1_BW_PER_PIXEL. ret %d\n", ret);
		hw_info->bw_per_pixel[AV1_NO_COMPRESS_BW] = 0;
	}

	ret = of_property_read_u32(dev->of_node, "HEVC_COMPRESS_BW_PER_PIXEL",
		&hw_info->bw_per_pixel[HEVC_COMPRESS_BW]);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get HEVC_COMPRESS_BW_PER_PIXEL. ret %d\n", ret);
		hw_info->bw_per_pixel[HEVC_COMPRESS_BW] = 0;
	}

	ret = of_property_read_u32(dev->of_node, "HEVC_BW_PER_PIXEL",
		&hw_info->bw_per_pixel[HEVC_NO_COMPRESS_BW]);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get HEVC_BW_PER_PIXEL. ret %d\n", ret);
		hw_info->bw_per_pixel[HEVC_NO_COMPRESS_BW] = 0;
	}

	ret = of_property_read_u32(dev->of_node, "VSD_BW_PER_PIXEL",
		&hw_info->bw_per_pixel[VSD_BW]);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get VSD_BW_PER_PIXEL. ret %d\n", ret);
		hw_info->bw_per_pixel[VSD_BW] = 0;
	}

	ret = of_property_read_u32(dev->of_node, "BW_EFFICIENCY", &efficiency);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get BW_EFFICIENCY. ret %d\n", ret);
		efficiency = 0;
	}

	for (i = 0; i < TOTAL_BW_COUNT; i++)
		hw_info->bw_per_pixel[i] =
			((hw_info->bw_per_pixel[i] * efficiency) / BW_EFFICIENCY_UNIT);

	dev_info(&pdev->dev, "hw count %d %d compress engine %d av1_compress_bw_per_pixel %d/1000 av1_bw_per_pixel %d/1000 hevc_compress_bw_per_pixel %d/1000 hevc_bw_per_pixel %d/1000 vsd_bw_per_pixel %d/1000 with efficiency %u/100\n",
		hw_info->core_count, hw_info->lat_count, hw_info->compress_engine,
		hw_info->bw_per_pixel[AV1_COMPRESS_BW],
		hw_info->bw_per_pixel[AV1_NO_COMPRESS_BW],
		hw_info->bw_per_pixel[HEVC_COMPRESS_BW],
		hw_info->bw_per_pixel[HEVC_NO_COMPRESS_BW],
		hw_info->bw_per_pixel[VSD_BW],
		efficiency);

	return 0;
}

static int vcu_parse_dtsi(struct platform_device *pdev, struct mtk_vcu *vcu)
{
	struct mtk_vcu_reg_param *reg_param = &vcu->reg_param;
	struct mtk_vcu_hw_info *hw_info = &vcu->hw_info;
	struct mtk_vcu_irq_param *irq_param = &vcu->irq_param;
	struct resource *res;
	struct device *dev = &pdev->dev;
	int i, ret;

	// Parse setting data
	ret = of_property_read_u32(dev->of_node, "CORE_COUNT", &hw_info->core_count);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get CORE_COUNT. ret %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev->of_node, "LAT_COUNT", &hw_info->lat_count);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get LAT_COUNT. ret %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev->of_node, "COMPRESS_ENGINE", &hw_info->compress_engine);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get COMPRESS_ENGINE. ret %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev->of_node, "NUM_OF_VDEC_BASE", &reg_param->reg_vdec_num);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get NUM_OF_VDEC_BASE. ret %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < MAX_REG_NUM; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (res == NULL) {
			dev_info(&pdev->dev, "get memory resource failed.\n");
			break;
		}

		reg_param->reg[i].va = (uintptr_t)devm_ioremap_resource(&pdev->dev, res);
		reg_param->reg[i].size = (unsigned int)resource_size(res);
		reg_param->reg[i].pa = (phys_addr_t)res->start;

		if (IS_ERR((__force void *)reg_param->reg[i].va)) {
			dev_err(&pdev->dev, "ioremap failed.\n");
			break;
		}
		dev_info(&pdev->dev, "[%d] vcu address %pa size 0x%x remap va 0x%lx\n",
			i, &reg_param->reg[i].pa, reg_param->reg[i].size, reg_param->reg[i].va);
	}
	reg_param->reg_num = i;
	if (reg_param->reg_num <= reg_param->reg_vdec_num) {
		dev_err(&pdev->dev, "wrong reg num %d < vdec num %d\n",
			reg_param->reg_num, reg_param->reg_vdec_num);
		return -EINVAL;
	}
	reg_param->reg_venc_num = reg_param->reg_num - reg_param->reg_vdec_num;

	dev_info(&pdev->dev, "reg num %d %d %d\n",
		reg_param->reg_num, reg_param->reg_vdec_num, reg_param->reg_venc_num);

	ret = of_property_read_u32(dev->of_node, "INT_NUM", &irq_param->irq_num);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get INT_NUM. ret %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < irq_param->irq_num; i++) {
		ret = platform_get_irq(pdev, i);
		if (ret < 0) {
			dev_info(&pdev->dev, "get %d irq failed ret %d\n", i, ret);
			break;
		}
		irq_param->irq_info[i].irq = ret;

		ret = of_property_read_string_index(dev->of_node, "interrupt-names", i,
						    &irq_param->irq_info[i].name);
		if (ret < 0) {
			dev_info(&pdev->dev, "get %d irq name failed ret %d\n", i, ret);
			break;
		}

		dev_info(&pdev->dev, "[%d] vcu irq %d name %s\n",
			i, irq_param->irq_info[i].irq, irq_param->irq_info[i].name);
	}

	dev_info(&pdev->dev, "irq num %d\n", irq_param->irq_num);

	vcu_parse_bw_info(pdev, vcu);
	vcu_parse_dv_profile_level(pdev, vcu);

	ret = of_property_read_u32(dev->of_node, "vdec_option_sel", &hw_info->vdec_option_sel);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get vdec_option_sel. ret %d\n", ret);
		return -EINVAL;
	}

	return 0;
}

static void mtk_tv_vcu_do_preset(struct mtk_vcu *vcu, const struct mtk_tv_vcu_ops *tv_vcu_ops)
{
	if (tv_vcu_ops && tv_vcu_ops->preset) {
		tv_vcu_ops->preset(PRESET_HW_INFO, &vcu->hw_info);
		tv_vcu_ops->preset(PRESET_REG, &vcu->reg_param);
		tv_vcu_ops->preset(PRESET_IRQ, &vcu->irq_param);
		tv_vcu_ops->preset(PRESET_DV_INFO, &vcu->dv_info_param);
		tv_vcu_ops->preset(PRESET_DEC_CAP_TABLE, &vcu->dec_cap_table);
		tv_vcu_ops->preset(PRESET_TEE_INTERFACE, &vcu->tee_interface);
	}
}

int vcu_register_tv_vcu_ops(enum vcu_codec_type type, const struct mtk_tv_vcu_ops *ops)
{
	struct device *dev;
	struct mtk_vcu *vcu;

	if (vcu_pdev == NULL) {
		dprintk(0, "vcu_pdev in NULL\n");
		return -EPROBE_DEFER;
	}

	if (ops == NULL) {
		dprintk(0, "tv vcu ops is NULL for type %d\n", type);
		return -EINVAL;
	}

	if (type >= VCU_CODEC_MAX || type < VCU_VDEC) {
		dprintk(0, "codec type %d is not valid\n", type);
		return -EINVAL;
	}

	dev = &vcu_pdev->dev;
	vcu = platform_get_drvdata(vcu_pdev);

	if (vcu == NULL) {
		dprintk(0, "vcu device in not ready\n");
		return -EPROBE_DEFER;
	}

	dev_info(dev, "register tv vcu ops for type %d\n", type);
	g_tv_vcu_ops[type] = ops;

	mtk_tv_vcu_do_preset(vcu, g_tv_vcu_ops[type]);
	g_tv_vcu_ops[type]->init(vcu_pdev, &vcu->sysfs_group[type]);
	mtk_vcu_create_sysfs_attr(vcu_pdev, &vcu->sysfs_group[type]);

	return 0;
}
EXPORT_SYMBOL(vcu_register_tv_vcu_ops);

static int mtk_vcu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_vcu *vcu;
	int ret = 0;

	vcu = devm_kzalloc(dev, sizeof(*vcu), GFP_KERNEL);
	if (!vcu)
		return -ENOMEM;

	dev_info(dev, "enter probe\n");

	vcu->dev = dev;
	platform_set_drvdata(pdev, vcu);
	mtk_vcu_update_iommu_buf_tag(vcu);
	ret = mtk_vcu_update_vdec_cap(vcu);
	if (ret < 0) {
		dev_err(dev, "update vdec cap fails ret %d\n", ret);
		return ret;
	}
	vcu_parse_dtsi(pdev, vcu);
	vcu_pdev = pdev;

	vcu->tee_interface.open_handler = vcu_tee_open_handler;
	vcu->tee_interface.close_handler = vcu_tee_close_handler;
	vcu->tee_interface.invoke_command = vcu_tee_invoke_command;
	vcu->tee_interface.register_shm = vcu_tee_register_shm;
	vcu->tee_interface.unregister_shm = vcu_tee_unregister_shm;

	return ret;
}

static const struct of_device_id mtk_vcu_match[] = {
	{.compatible = "mediatek,tv-vcu",},
	{},
};

MODULE_DEVICE_TABLE(of, mtk_vcu_match);

static int mtk_vcu_remove(struct platform_device *pdev)
{
	struct mtk_vcu *vcu = platform_get_drvdata(pdev);

	mtk_vcu_remove_sysfs_attr(pdev, &vcu->sysfs_group[VCU_VDEC]);
	mtk_vcu_remove_sysfs_attr(pdev, &vcu->sysfs_group[VCU_VENC]);
	if (g_tv_vcu_ops[VCU_VDEC])
		g_tv_vcu_ops[VCU_VDEC]->deinit(pdev);
	if (g_tv_vcu_ops[VCU_VENC])
		g_tv_vcu_ops[VCU_VENC]->deinit(pdev);

	if (vcu->dec_cap_table.data)
		devm_kfree(&pdev->dev, vcu->dec_cap_table.data);
	vcu->dec_cap_table.data = NULL;
	vcu->dec_cap_table.size = 0;

	devm_kfree(&pdev->dev, vcu);

	return 0;
}

static int mtk_vcu_suspend(struct device *dev)
{
	int ret = 0;

	if (g_tv_vcu_ops[VCU_VDEC])
		ret = g_tv_vcu_ops[VCU_VDEC]->suspend(dev);
	// no need for vcu_enc for now

	return ret;
}

static int mtk_vcu_resume(struct device *dev)
{
	int ret = 0;

	if (g_tv_vcu_ops[VCU_VDEC])
		ret = g_tv_vcu_ops[VCU_VDEC]->resume(dev);
	// no need for vcu_enc for now

	return ret;
}

static const struct dev_pm_ops mtk_vcu_pm_ops = {
	.suspend = mtk_vcu_suspend,
	.resume = mtk_vcu_resume,
};

static struct platform_driver mtk_vcu_driver = {
	.probe  = mtk_vcu_probe,
	.remove = mtk_vcu_remove,
	.driver = {
		.name   = "mtk_tv_vcu",
		.pm = &mtk_vcu_pm_ops,
		.owner  = THIS_MODULE,
		.of_match_table = mtk_vcu_match,
	},
};

module_platform_driver(mtk_vcu_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Mediatek Video Communication And Controller Unit driver");
