<div class="p-8 grid grid-cols-6">

    <ol class="breadcrumb col-span-full mb-8">
        
        <li class="crumb"><a class="anchor" href="/adminApp">Admin App</a></li>
        <li class="crumb-separator" aria-hidden>/</li>
        <li class="crumb"><a class="anchor" href="/adminApp/departments">Departments</a></li>
        <li class="crumb-separator" aria-hidden>/</li>
        <li>New</li>
    </ol>
					
    <form class="col-start-2 col-span-4">
        <div class="space-y-12">
            <div class="border-b border-gray-900/10 pb-12">
        <h2 class="text-base font-semibold leading-7 text-gray-900">Department</h2>
  
        <div class="mt-10 grid grid-cols-1 gap-x-6 gap-y-8 sm:grid-cols-6">
          <div class="sm:col-span-4">
            <label for="username" class="block text-sm font-medium leading-6 text-gray-900">ID</label>
            <div class="mt-2">
              <div class="flex rounded-md shadow-sm ring-1 ring-inset ring-gray-300 focus-within:ring-2 focus-within:ring-inset focus-within:ring-indigo-600 sm:max-w-md">
                <span class="flex select-none items-center pl-3 text-gray-500 sm:text-sm">department:</span>
                <input required type="text" name="username" id="username" autocomplete="username" class="block flex-1 border-0 bg-transparent py-1.5 pl-1 text-gray-900 placeholder:text-gray-400 focus:ring-0 sm:text-sm sm:leading-6" placeholder="janesmith" pattern="[A-Za-z]{3}" title="This must be of 3 letters">
              </div>
            </div>
          </div>
        </div>
      </div>
  
      <div class="border-b border-gray-900/10 pb-12">
        <h2 class="text-base font-semibold leading-7 text-gray-900">Details</h2>
  
        <div class="mt-10 grid grid-cols-1 gap-x-6 gap-y-8 sm:grid-cols-6">
          <div class="sm:col-span-3">
            <label class="label">
                <span class="text-sm font-medium text-gray-900 leading-6">Name</span>
                <input required class="input mt-2 rounded-md py-1.5 shadow-sm" type="text" placeholder="Name" name="name" id="name"/>
            </label>
          </div>
            
          <div class="sm:col-span-3">
            <label class="label">
                <span class="text-sm font-medium text-gray-900 leading-6">Building</span>
                <input required class="input mt-2 rounded-md py-1.5 shadow-sm" type="text" placeholder="Building" name="building" id="building"/>
            </label>
          </div>
  
          <div class="sm:col-span-4">
            <label class="label">
                <span class="text-sm font-medium text-gray-900 leading-6">Email Address</span>
                <input required class="input mt-2 rounded-md py-1.5 shadow-sm" type="email" placeholder="Email" name="email" id="email"/>
            </label>
          </div>
  
          <div class="sm:col-span-2 sm:col-start-1">
            <label class="label">
                <span class="text-sm font-medium text-gray-900 leading-6">Code</span>
                <input required class="input mt-2 rounded-md py-1.5 shadow-sm" type="number" placeholder="Code" name="code" id="code"/>
            </label>
          </div>
  
          <div class="sm:col-span-2">
            <label class="label">
                <span class="text-sm font-medium text-gray-900 leading-6">Floor</span>
                <input required class="input mt-2 rounded-md py-1.5 shadow-sm" type="number" placeholder="Floor" name="floor" id="floor"/>
            </label>
          </div>
  
          <div class="sm:col-span-2">
            <label class="label">
                <span class="text-sm font-medium text-gray-900 leading-6">Minor Course Code</span>
                <input required class="input mt-2 rounded-md py-1.5 shadow-sm" type="text" placeholder="Minor Course Code" name="minor-course-code" id="minor-course-code" pattern="[A-Za-z]{1}" title="This must be of 1 letter"/>
            </label>
          </div>
        </div>
      </div>
  
      
    </div>
  
    <div class="mt-6 flex items-center justify-end gap-x-6">
      <button type="button" class="text-sm font-semibold leading-6 text-gray-900">Cancel</button>
      <button type="submit" class="rounded-md bg-indigo-600 px-3 py-2 text-sm font-semibold text-white shadow-sm hover:bg-indigo-500 focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-indigo-600">Save</button>
    </div>
  </form>
  
</div>