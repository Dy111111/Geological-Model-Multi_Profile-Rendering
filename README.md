# Geological-3D-Model-Multi_Profile-Rendering
使用OpenGL实现的基于像素链表的地质体模型多剖面实时渲染方法，主要用到了着色器缓存对象和原子交换操作

主要包括以下步骤：
- 着色器缓存对象分配链表空间
- 构建像素链表并写入
- 渲染剖面，遍历像素链表，写入剖面颜色
<div align="center">
<img src="https://github.com/user-attachments/assets/f118b16e-4def-4633-a2de-be598160e458" width="50%">
</div>
<br />
<div align="center">
<img src="https://github.com/user-attachments/assets/1c25a6c5-da44-4976-9308-aa43bc951453" width="30%">
<img src="https://github.com/user-attachments/assets/7c117adc-ee1a-4780-8b93-aea49369a15b" width="30%">
</div>

参考：
- Real-time concurrent linked list construction on the GPU Jason C. Yang, Justin Hensley, Holger Grün and Nicolas Thibieroz http://dl.acm.org/citation.cfm?id=2383624

- THIBIEROZ, NICOLAS. “Order-Independent Transparency using Per-Pixel Linked Lists”. GPU Pro 2. Ed. by ENGEL, WOLFGANG. A K Peters, 2011, 409–431 2, 10.

