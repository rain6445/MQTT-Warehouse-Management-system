<template>
  <div class="content-box">
    <h2>当前板块：{{ activeTab }}</h2>
    <p>这里是 {{ activeTab }} 的详细内容展示区域。你可以在这里添加你的静态网页内容。</p>
  </div>
</template>

<script setup>
defineProps({
  activeTab: String
})
</script>

<style scoped>
.content-box {
  padding: 40px;
  text-align: center;
  background-color: #f9f9f9;
  border-radius: 8px;
  margin-top: 20px;
  box-shadow: 0 2px 12px 0 rgba(0,0,0,0.1);
}
</style>
