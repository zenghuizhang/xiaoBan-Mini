import React from 'react';
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import { RobotUI } from './RobotUI';

const App = () => {
  return (
    <BrowserRouter>
      <div className="min-h-screen w-full bg-zinc-950 flex items-center justify-center">
        {/* Container to simulate the device screen */}
        <div className="shadow-2xl ring-1 ring-white/10 rounded-sm overflow-hidden">
          <Routes>
            <Route path="/" element={<RobotUI />} />
            <Route path="*" element={<Navigate to="/?state=boot&theme=tech" replace />} />
          </Routes>
        </div>
      </div>
    </BrowserRouter>
  );
};

export default App;