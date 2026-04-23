<script lang="ts">
  import { onMount } from 'svelte';

  let todos: any[] = $state([]);
  let newTitle = $state('');

  const loadTodos = async () => {
    try {
      const response = await fetch('http://localhost:8080/todo');
      if (response.ok) {
        todos = await response.json();
      }
    } catch (error) {
      console.error(error);
    }
  };

  onMount(loadTodos);

  const addTodo = async () => {
    if (!newTitle.trim()) return;
    try {
      const response = await fetch('http://localhost:8080/todo', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ title: newTitle, completed: false })
      });
      if (response.ok) {
        newTitle = '';
        await loadTodos();
      }
    } catch (error) {
      console.error(error);
    }
  };

  const toggleTodo = async (todo: any) => {
    try {
      const response = await fetch(`http://localhost:8080/todo/${todo.id}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ completed: !todo.completed })
      });
      if (response.ok) {
        await loadTodos();
      }
    } catch (error) {
      console.error(error);
    }
  };

  const deleteTodo = async (id: number) => {
    try {
      const response = await fetch(`http://localhost:8080/todo/${id}`, {
        method: 'DELETE'
      });
      if (response.ok) {
        await loadTodos();
      }
    } catch (error) {
      console.error(error);
    }
  };
</script>

<main style="max-width: 600px; margin: 0 auto; padding: 20px; font-family: sans-serif;">
  <h1 style="color: #333; text-align: center;">Super Lista de Tareas</h1>
  
  <div style="display: flex; gap: 10px; margin-bottom: 20px;">
    <input 
      type="text" 
      bind:value={newTitle} 
      placeholder="Escribe una nueva tarea y presiona enter..." 
      style="flex: 1; padding: 10px; font-size: 16px; border: 1px solid #ccc; border-radius: 4px;"
      onkeydown={(e) => e.key === 'Enter' ? addTodo() : null}
    />
    <button onclick={addTodo} style="padding: 10px 20px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer; font-weight: bold;">
      AGREGAR
    </button>
  </div>

  <ul style="list-style: none; padding: 0; display: flex; flex-direction: column; gap: 8px;">
    {#each todos as todo}
      <li style="display: flex; align-items: center; gap: 15px; padding: 15px; border-radius: 8px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); background: white;">
        <input 
          type="checkbox" 
          checked={todo.completed} 
          onchange={() => toggleTodo(todo)}
          style="width: 20px; height: 20px; cursor: pointer;"
        />
        <span style="flex: 1; font-size: 18px; text-decoration: {todo.completed ? 'line-through' : 'none'}; color: {todo.completed ? '#888' : '#333'};">
          {todo.title}
        </span>
        <button onclick={() => deleteTodo(todo.id)} style="padding: 8px 12px; background: #ff4757; color: white; border: none; border-radius: 4px; cursor: pointer; font-weight: bold;">
          ✕ BORRAR
        </button>
      </li>
    {:else}
      <div style="text-align: center; padding: 20px; background: #f9f9f9; border-radius: 8px; color: #777;">
        <strong>Excelente.</strong> ¡No tienes tareas pendientes ahora mismo!
      </div>
    {/each}
  </ul>
</main>

